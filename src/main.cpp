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

#if !defined(__cplusplus) || __cplusplus < 202302L
#error This code requires C++23 or later.
#endif

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
import Profiler;
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
import Profiler;
import StrategyProvider;
import Properties;

[[noreturn]] auto autoInvocation(std::unique_ptr<ImplementedGraph> g) -> void;

auto getProfilingLevel(const std::vector<std::string_view>& args) -> int {
    int profilingLevel = 3;
    const auto profilingIt = std::ranges::find(args, "--profiling");
    const bool isProfilingMode = profilingIt != args.end();
    if (!isProfilingMode)
        return 0;

    const auto levelIt = profilingIt + 1;
    if (levelIt != args.end()) {
        std::string levelStr(levelIt->data(), levelIt->size());
        if (levelStr.starts_with("--")) {
            goto noprofilingarg;
        }
        try {
            size_t pos;
            int level = std::stoi(levelStr, &pos);
            if (pos == levelStr.length()) {
                profilingLevel = level;
                if (profilingLevel < 0) {
                    std::cerr << "Profiling level bad" << std::endl;
                    exit(1);
                }
            } else {
                std::cerr << "Profiling level bad" << std::endl;
                exit(1);
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Profiling level bad" << std::endl;
            exit(1);
        } catch (const std::out_of_range& e) {
            std::cerr << "Profiling level bad" << std::endl;
            exit(1);
        }
    }

    noprofilingarg:
    return profilingLevel;
}

int main(int argc, char* argv[]) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    const bool isGeneratorMode = std::ranges::find(args, "--generator") != args.end();
    // const bool isProfilingMode = std::ranges::find(args, "--profiling") != args.end();
    int profilingLevel = getProfilingLevel(args);
    bool isProfilingMode = profilingLevel > 0;

    if (isGeneratorMode) {
        constexpr int vertex_count = 100;
        constexpr int edge_count = 5000;
        std::cout << vertex_count << std::endl;
        const auto edges = generate_erdos_renyi_edges(vertex_count, edge_count);
        for (const auto& [i,j] : edges) {
            std::cout << i << " " << j << "\n";
        }
        return 0;
    }

    constexpr bool isDebugMode =
#ifdef DEBUG
    true;
#else
        false;
#endif

    if (isDebugMode || isGeneratorMode) std::cout << "Generator Mode: " << (isGeneratorMode ? "true" : "false") << std::endl;
    if (isDebugMode || isProfilingMode) std::cout << "Profiling Mode: " << (isProfilingMode ? "true" : "false") << std::endl;
    if (isDebugMode || isProfilingMode) std::cout << "Profiling Level: " << profilingLevel << std::endl;



    if (isProfilingMode) {
        runProfilingMode<isDebugMode>(profilingLevel, true);
        return 0;
    }


    if (isDebugMode) {
        std::cout << "[DEBUG] Debug mode enabled.\n\n";
    }

    int n;
    if (isDebugMode) std::cout << "\n[DEBUG] Vertex count (n): ";
    std::cin >> n;
    if (!std::cin || n < 0) {
        std::cerr << "Invalid vertex count." << std::endl;
        return 1;
    }
    auto g = GraphFactory<ImplementedGraph>::createGraph<GraphNList>(n);
    if (!g) {
        std::cerr << "Graph creation failed";
        return 1;
    }
    if (isDebugMode) std::cout << "\n[DEBUG] Edges (u v pairs), to finish type EOF (Ctrl+D Linux/macOS, Ctrl+Z Windows):" << std::endl;
    int u, v;
    while (std::cin >> u >> v) {
        g->addEdge(u, v);
    }
    std::cin.clear();

    if (isDebugMode) std::cout << "\n[DEBUG] Graph structure:\n" << *g << "\n";

    if (isDebugMode) std::cout << "autoinvocation" << std::endl;
    autoInvocation(std::move(g));

    using ProcessorType = GraphProcessor<ImplementedGraph, IAlgorithm<ImplementedGraph>, isDebugMode>;
    ProcessorType processor;

    std::vector<std::unique_ptr<IAlgorithm<ImplementedGraph>>> algorithms_to_run;

    algorithms_to_run.push_back(make_decorated_algorithm<ProcessorType::SourceVertexStrategy>());
    if (isDebugMode) algorithms_to_run.push_back(make_decorated_algorithm<ProcessorType::SequentialDiameterStrategy>());
    algorithms_to_run.push_back(make_decorated_algorithm<ProcessorType::AsyncDiameterStrategy>());
    if (isDebugMode) algorithms_to_run.push_back(make_decorated_algorithm<ProcessorType::ParallelDiameterStrategy>());
    algorithms_to_run.push_back(make_decorated_algorithm<ProcessorType::FeedbackArcSetRemoveCyclesStrategy>());
    algorithms_to_run.push_back(make_decorated_algorithm<ProcessorType::FeedbackArcSetInsertEdgesStrategy>());
    algorithms_to_run.push_back(make_decorated_algorithm<ProcessorType::FeedbackArcSetDfsStrategy>());
    if (isDebugMode) algorithms_to_run.push_back(make_decorated_algorithm<ProcessorType::SequentialUniversalSourceFinderStrategy>());
    if (isDebugMode) algorithms_to_run.push_back(make_decorated_algorithm<ProcessorType::ParallelUniversalSourceFinderStrategy>());
    algorithms_to_run.push_back(make_decorated_algorithm<ProcessorType::KosarajuUniversalSourceFinderStrategy>());
    algorithms_to_run.push_back(make_decorated_algorithm<ProcessorType::TarjanUniversalSourceFinderStrategy>());
    algorithms_to_run.push_back(make_decorated_algorithm<ProcessorType::PathBasedUniversalSourceFinderStrategy>());

    for (const auto& algo : algorithms_to_run) {
        auto result = algo->execute(*g);
        printResult<std::ostream>(result, std::cout);
    }
    // g->convertTo<GraphFList>();
    // for (const auto& algo : algorithms_to_run) {
    //     auto result = algo->execute(*g);
    //     printResult<std::ostream>(result, std::cout);
    // }

    return 0;
}

[[noreturn]] auto autoInvocation(std::unique_ptr<ImplementedGraph> g) -> void {
    constexpr bool isDebugMode =
#ifdef DEBUG
    true;
#else
    false;
#endif
    using StandardStrategySelector = typename StrategyProvider<ImplementedGraph, IAlgorithm<ImplementedGraph>, isDebugMode>::type;

    if (isDebugMode) std::cout << "\n--- Solving Problem 1: Source Vertex Count ---\n";
    auto p1_result = StandardStrategySelector::solve<Problem::SourceVertexCount, isDebugMode>(*g);
    printResult(p1_result);

    if (isDebugMode) std::cout << "\n--- Solving Problem 2: Diameter Measure ---\n";
    auto p2_result = StandardStrategySelector::solve<Problem::DiameterMeasure, isDebugMode>(*g);
    printResult(p2_result);

    if (isDebugMode) std::cout << "\n--- Solving Problem 3: Feedback Arc Set ---\n";
    auto p3_result = StandardStrategySelector::solve<Problem::FeedbackArcSet, isDebugMode>(*g);
    printResult(p3_result);

    if (isDebugMode) std::cout << "\n--- Solving Problem 4: First Universal Source ---\n";
    auto p4_result = StandardStrategySelector::solve<Problem::FirstUniversalSource, isDebugMode>(*g);
    printResult(p4_result);
    exit(0);
}