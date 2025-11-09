#include <catch2/catch_template_test_macros.hpp>
#include <vector>
#include <memory>
#include <variant>
#include <random>

import ImplementedGraph;
import GraphNList;
import GraphFList;
import GraphAMatrix;
import GraphFactory;
import GraphAlgo;
import AlgorithmResult;
import Generator;

TEMPLATE_TEST_CASE("Universal Source Vertex Calculation", "[universal_source]", GraphNList, GraphFList, GraphAMatrix) {
    using IGraphPtr = std::unique_ptr<ImplementedGraph>;
    using GraphType = TestType;

    GraphProcessor<ImplementedGraph>::SequentialUniversalSourceFinderStrategy seq_strategy;
    GraphProcessor<ImplementedGraph>::ParallelUniversalSourceFinderStrategy par_strategy;
    GraphProcessor<ImplementedGraph>::KosarajuUniversalSourceFinderStrategy kosaraju_strategy;
    GraphProcessor<ImplementedGraph>::TarjanUniversalSourceFinderStrategy tarjan_strategy;
    GraphProcessor<ImplementedGraph>::PathBasedUniversalSourceFinderStrategy path_based_strategy;

    auto test_all_strategies = [&](const ImplementedGraph& g, int expected_source) {
        auto seq_result = std::get<int>(seq_strategy.execute(g));
        auto par_result = std::get<int>(par_strategy.execute(g));
        auto kosaraju_result = std::get<int>(kosaraju_strategy.execute(g));
        auto tarjan_result = std::get<int>(tarjan_strategy.execute(g));
        auto path_based_result = std::get<int>(path_based_strategy.execute(g));

        REQUIRE(seq_result == expected_source);
        REQUIRE(par_result == expected_source);
        REQUIRE(kosaraju_result == expected_source);
        REQUIRE(tarjan_result == expected_source);
        REQUIRE(path_based_result == expected_source);
    };

    SECTION("Trivial Cases") {
        SECTION("Empty graph has no universal source") {
            IGraphPtr g0 = GraphFactory<ImplementedGraph>::createGraph<GraphType>(0);
            test_all_strategies(*g0, -1);
        }

        SECTION("Graph with a single vertex") {
            IGraphPtr g1 = GraphFactory<ImplementedGraph>::createGraph<GraphType>(1);
            test_all_strategies(*g1, 0);
        }
    }

    SECTION("Graphs WITH a Universal Source") {
        SECTION("Line or Path graph") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(5);
            g->addEdge(0, 1);
            g->addEdge(1, 2);
            g->addEdge(2, 3);
            g->addEdge(3, 4);
            test_all_strategies(*g, 0);
        }

        SECTION("Source vertex that feeds into a cycle") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(5);
            g->addEdge(0, 1); // 0 is the source
            g->addEdge(1, 2);
            g->addEdge(2, 3);
            g->addEdge(3, 1);
            g->addEdge(2, 4); // Path out of the cycle
            test_all_strategies(*g, 0);
        }

        SECTION("Strongly Connected Cycle") {
            int n = 5;
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(n);
            for (int i = 0; i < n; ++i) {
                g->addEdge(i, (i + 1) % n);
            }
            test_all_strategies(*g, 0);
        }

        SECTION("Complete Directed Graph") {
            int n = 5;
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(n);
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    if (i != j) g->addEdge(i, j);
                }
            }
            test_all_strategies(*g, 0);
        }

        SECTION("Source SCC with multiple vertices") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(5);
            g->addEdge(1, 2);
            g->addEdge(2, 1);
            g->addEdge(1, 0);
            g->addEdge(2, 3);
            g->addEdge(3, 4);
            test_all_strategies(*g, 1);
        }
    }

    SECTION("Graphs with NO Universal Source") {
        SECTION("Disconnected graph") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(6);
            g->addEdge(0, 1); g->addEdge(1, 2); g->addEdge(2, 0);
            g->addEdge(3, 4); g->addEdge(4, 5); g->addEdge(5, 3);
            test_all_strategies(*g, -1);
        }

        SECTION("Isolated vertex") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(4);
            g->addEdge(1, 0);
            g->addEdge(2, 0);
            g->addEdge(3, 0);
            test_all_strategies(*g, -1);
        }

        SECTION("DAG with two sources") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(4);
            g->addEdge(0, 2);
            g->addEdge(1, 2);
            g->addEdge(2, 3);
            test_all_strategies(*g, -1);
        }

        SECTION("Graph with two source SCCs") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(4);
            g->addEdge(0, 1); g->addEdge(1, 0);
            g->addEdge(2, 3); g->addEdge(3, 2);
            test_all_strategies(*g, -1);
        }

        SECTION("Cycle with an unreachable node") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(4);
            g->addEdge(0, 1);
            g->addEdge(1, 2);
            g->addEdge(2, 0);
            test_all_strategies(*g, -1);
        }
    }

    SECTION("Randomized graph checks for consistency") {
        constexpr int num_tests = 20;
        constexpr int num_vertices = 15;
        constexpr int min_edges = 10;
        constexpr int max_edges = 250;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> edge_dist(min_edges, max_edges);

        for (int i = 0; i < num_tests; ++i) {
            const int num_edges = edge_dist(gen);
            auto g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(num_vertices);
            auto edges = generate_erdos_renyi_edges(num_vertices, num_edges);

            for (const auto& edge : edges) {
                g->addEdge(edge.first, edge.second);
            }

            // sequential is the truth
            const auto expected_result = std::get<int>(seq_strategy.execute(*g));

            const auto par_result = std::get<int>(par_strategy.execute(*g));
            const auto kosaraju_result = std::get<int>(kosaraju_strategy.execute(*g));
            const auto tarjan_result = std::get<int>(tarjan_strategy.execute(*g));
            const auto path_based_result = std::get<int>(path_based_strategy.execute(*g));


            INFO("Test run " << i + 1 << " with " << num_vertices << " vertices and " << num_edges << " edges.");
            REQUIRE(par_result == expected_result);
            REQUIRE(kosaraju_result == expected_result);
            REQUIRE(tarjan_result == expected_result);
            REQUIRE(path_based_result == expected_result);
        }
    }
}