module;

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <set>
#include <map>
#include <optional>
#include <cmath>
#include <sstream>
#include <fstream>

export module Profiler;

import GraphNList;
import GraphFList;
import GraphAMatrix;
import GraphAlgo;
import ImplementedGraph;
import AlgorithmResult;
import GraphFactory;
import IAlgorithm;
import AlgorithmDecorator;
import DecoratorFactory;
import Generator;
import GraphProcessorAlgorithmStrategyFactory;

export void printProfilingResults(
    const std::map<std::string, std::map<std::string, std::optional<double>>>& results,
    const std::vector<std::pair<int, int>>& tested_sizes
) {
    std::cout << "\n\n--- Profiling Results (times in seconds) ---" << std::endl;

    std::vector<std::string> algo_names;
    if (!results.empty()) {
        for (const auto& pair : results) {
            algo_names.push_back(pair.first);
        }
        std::sort(algo_names.begin(), algo_names.end());
    }

    std::vector<std::string> size_keys;
    for(const auto& size_pair : tested_sizes) {
         size_keys.push_back("V:" + std::to_string(size_pair.first) + ",E:" + std::to_string(size_pair.second));
    }

    const int name_width = 40;
    const int time_width = 20;
    std::cout << "| " << std::setw(name_width) << std::left << "Algorithm" << " |";
    for (const auto& key : size_keys) {
        std::cout << " " << std::setw(time_width) << std::left << key << " |";
    }
    std::cout << std::endl;
    std::cout << "|-" << std::string(name_width, '-') << "-|";
    for (size_t i = 0; i < size_keys.size(); ++i) {
        std::cout << "-" << std::string(time_width, '-') << "-|";
    }
    std::cout << std::endl;
    for (const auto& name : algo_names) {
        std::cout << "| " << std::setw(name_width) << std::left << name << " |";
        const auto& algo_results = results.at(name);
        for (const auto& key : size_keys) {
            std::stringstream ss;
            if (auto it = algo_results.find(key); it != algo_results.end()) {
                if (it->second.has_value()) {
                    ss << std::fixed << std::setprecision(4) << it->second.value();
                } else {
                    ss << "TIMEOUT/OOM";
                }
            } else {
                ss << "N/A";
            }
            std::cout << " " << std::setw(time_width) << std::left << ss.str() << " |";
        }
        std::cout << std::endl;
    }
}

export void printProfilingResultsCSV(
    const std::map<std::string, std::map<std::string, std::optional<double>>>& results,
    const std::vector<std::pair<int, int>>& tested_sizes,
    const std::string& filename = "profiling_results.csv"
) {
    std::ofstream csv_file(filename);
    if (!csv_file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    std::cout << "\n--- Writing profiling results to " << filename << " ---" << std::endl;

    std::vector<std::string> algo_names;
    if (!results.empty()) {
        for (const auto& pair : results) {
            algo_names.push_back(pair.first);
        }
        std::sort(algo_names.begin(), algo_names.end());
    }

    std::vector<std::string> size_keys;
    for(const auto& size_pair : tested_sizes) {
        size_keys.push_back("V:" + std::to_string(size_pair.first) + ",E:" + std::to_string(size_pair.second));
    }

    // header
    csv_file << "Algorithm";
    for (const auto& key : size_keys) {
        csv_file << ",\"" << key << "\"";
    }
    csv_file << "\n";

    // rows
    for (const auto& name : algo_names) {
        csv_file << name;
        const auto& algo_results = results.at(name);
        for (const auto& key : size_keys) {
            csv_file << ",";
            if (auto it = algo_results.find(key); it != algo_results.end()) {
                if (it->second.has_value()) {
                    csv_file << std::fixed << std::setprecision(6) << it->second.value();
                } else {
                    csv_file << "TIMEOUT/OOM";
                }
            } else {
                csv_file << "N/A";
            }
        }
        csv_file << "\n";
    }
    csv_file.close();
}

export template <bool isDebugMode>
void runProfilingMode(int profilinglevel = 3, bool use_csv = false) {
    std::cout << "Starting profiling mode..." << std::endl;
    if constexpr (isDebugMode) {
        std::cout << "[INFO] Profiling a DEBUG build." << std::endl;
    }
    std::vector<int> steps;
    for (int i = 0; i <= profilinglevel; ++i) {
        long long val = static_cast<long long>(std::pow(10, i));
        steps.push_back(val);
    }
    if (steps.front() != 1) steps.insert(steps.begin(), 1);
    std::map<std::string, std::map<std::string, std::optional<double>>> results;
    std::set<std::string> eliminated_algos;
    std::vector<std::pair<int, int>> tested_sizes;
    using IAlgo = IAlgorithm<ImplementedGraph>;
    using ProcessorType = GraphProcessor<ImplementedGraph, IAlgo, isDebugMode>;
    using Factory = GraphProcessorAlgorithmStrategyFactory<ImplementedGraph, IAlgo, isDebugMode>;
    std::vector<std::unique_ptr<IAlgo>> algorithms;
    Factory factory;
    algorithms.push_back(factory.createSourceVertexStrategy());
    if constexpr (isDebugMode) {
        algorithms.push_back(factory.createSequentialDiameterStrategy());
        algorithms.push_back(factory.createParallelDiameterStrategy());
        algorithms.push_back(factory.createSequentialUniversalSourceFinderStrategy());
        algorithms.push_back(factory.createParallelUniversalSourceFinderStrategy());
    }
    algorithms.push_back(factory.createAsyncDiameterStrategy());
    algorithms.push_back(factory.createFeedbackArcSetRemoveCyclesStrategy());
    algorithms.push_back(factory.createFeedbackArcSetInsertEdgesStrategy());
    algorithms.push_back(factory.createFeedbackArcSetDfsStrategy());
    algorithms.push_back(factory.createKosarajuUniversalSourceFinderStrategy());
    algorithms.push_back(factory.createTarjanUniversalSourceFinderStrategy());
    algorithms.push_back(factory.createPathBasedUniversalSourceFinderStrategy());

    using EdgeList = std::vector<std::pair<int, int>>;
    std::map<std::string, std::function<std::unique_ptr<ImplementedGraph>(int, const EdgeList&)>> graph_factories;

    graph_factories["GraphNList"] = [](int v_count, const EdgeList& edges) {
        auto g = GraphFactory<ImplementedGraph>::createGraph<GraphNList>(v_count);
        for (const auto& edge : edges) {
            g->addEdge(edge.first, edge.second);
        }
        return g;
    };
    graph_factories["GraphFList"] = [](int v_count, const EdgeList& edges) {
        GraphNList temp_g(v_count);
        for (const auto& edge : edges) {
            temp_g.addEdge(edge.first, edge.second);
        }
        return std::make_unique<ImplementedGraph>(GraphFList(temp_g));
    };
     graph_factories["GraphAMatrix"] = [](int v_count, const EdgeList& edges) {
        GraphNList temp_g(v_count);
        for (const auto& edge : edges) {
            temp_g.addEdge(edge.first, edge.second);
        }
        return std::make_unique<ImplementedGraph>(GraphAMatrix(temp_g));
    };


    for (int v_count : steps) {
        std::vector<int> edge_steps;
        int vlog = std::log10(v_count);
        for (int i = 0; i <= vlog*2; ++i) {
            int val = static_cast<int>(std::pow(10, i));
            edge_steps.push_back(val);
        }
        for (int e_count : edge_steps) {
            if (v_count == 0) continue;

            std::cout << "\n--- Testing with V=" << v_count << ", E=" << e_count << " ---" << std::endl;
            tested_sizes.push_back({v_count, e_count});
            std::string size_key = "V:" + std::to_string(v_count) + ",E:" + std::to_string(e_count);

            auto edges = generate_erdos_renyi_edges(v_count, e_count);

            for (const auto& [graph_name, graph_builder] : graph_factories) {
                std::cout << "  Testing on implementation: " << graph_name << std::endl;

                std::unique_ptr<ImplementedGraph> g;
                try {
                     g = graph_builder(v_count, edges);
                } catch (const std::bad_alloc& e) {
                    std::cout << "    - Graph creation failed (Out of Memory). Skipping for this implementation." << std::endl;
                    for (const auto& algo : algorithms) {
                         std::string combined_name = std::string(algo->getName()) + "_on_" + graph_name;
                         results[combined_name][size_key] = std::nullopt;
                    }
                    continue; // Skip to the next graph impl
                }

                for (const auto& algo : algorithms) {
                    const std::string combined_name = std::string(algo->getName()) + "_on_" + graph_name;

                    if (eliminated_algos.contains(combined_name)) {
                        std::cout << "    - Skipping " << std::setw(20) << std::left << algo->getName() << " (eliminated)" << std::endl;
                        results[combined_name][size_key] = std::nullopt;
                        continue;
                    }

                    try {
                        const auto start = std::chrono::steady_clock::now();
                        auto result = algo->execute(*g);
                        const auto finish = std::chrono::steady_clock::now();
                        const std::chrono::duration<double> elapsed = finish - start;
                        double elapsed_sec = elapsed.count();

                        std::cout << "    - " << std::setw(20) << std::left << algo->getName() << " finished in "
                                  << std::fixed << std::setprecision(6) << elapsed_sec << "s" << std::endl;

                        results[combined_name][size_key] = elapsed_sec;

                        if (elapsed_sec > 5.0) {
                            std::cout << "      -> Eliminating " << combined_name << " for future runs." << std::endl;
                            eliminated_algos.insert(combined_name);
                        }
                    } catch (const std::bad_alloc& e) {
                        std::cout << "    - " << std::setw(20) << std::left << algo->getName() << " failed with std::bad_alloc (Out of Memory)" << std::endl;
                        results[combined_name][size_key] = std::nullopt;
                        eliminated_algos.insert(combined_name);
                    } catch (const std::exception& e) {
                         std::cout << "    - " << std::setw(20) << std::left << algo->getName() << " failed with exception: " << e.what() << std::endl;
                        results[combined_name][size_key] = std::nullopt;
                        eliminated_algos.insert(combined_name);
                    }
                }
            }
        }
        eliminated_algos.clear();
    }

    if (use_csv)
        printProfilingResultsCSV(results, tested_sizes);
    printProfilingResults(results, tested_sizes);
}