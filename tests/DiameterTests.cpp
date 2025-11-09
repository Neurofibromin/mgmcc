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

TEMPLATE_TEST_CASE("Graph Diameter Calculation", "[diameter]", GraphNList, GraphFList, GraphAMatrix) {
    using IGraphPtr = std::unique_ptr<ImplementedGraph>;
    using GraphType = TestType;

    GraphProcessor<ImplementedGraph>::SequentialDiameterStrategy seq_strategy;
    GraphProcessor<ImplementedGraph>::AsyncDiameterStrategy async_strategy;
    GraphProcessor<ImplementedGraph>::ParallelDiameterStrategy par_strategy;

    auto test_all_strategies = [&](const ImplementedGraph& g, int expected_diameter) {
        auto seq_result = std::get<int>(seq_strategy.execute(g));
        auto async_result = std::get<int>(async_strategy.execute(g));
        auto par_result = std::get<int>(par_strategy.execute(g));
        REQUIRE(seq_result == expected_diameter);
        REQUIRE(async_result == expected_diameter);
        REQUIRE(par_result == expected_diameter);
    };

    SECTION("Trivial Cases") {
        SECTION("Empty graph has a diameter of 0") {
            IGraphPtr g0 = GraphFactory<ImplementedGraph>::createGraph<GraphType>(0);
            test_all_strategies(*g0, 0);
        }

        SECTION("Graph with a single vertex has a diameter of 0") {
            IGraphPtr g1 = GraphFactory<ImplementedGraph>::createGraph<GraphType>(1);
            test_all_strategies(*g1, 0);
        }
    }

    SECTION("Not Strongly Connected Graphs (Expect -1)") {
        SECTION("Disconnected graph with no edges") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(5);
            test_all_strategies(*g, -1);
        }

        SECTION("Simple Directed Acyclic Graph (DAG)") {
            IGraphPtr g_dag = GraphFactory<ImplementedGraph>::createGraph<GraphType>(4);
            g_dag->addEdge(0, 1);
            g_dag->addEdge(0, 2);
            g_dag->addEdge(1, 3);
            test_all_strategies(*g_dag, -1);
        }

        SECTION("Graph with two separate components") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(6);
            // Component 1
            g->addEdge(0, 1);
            g->addEdge(1, 0);
            // Component 2
            g->addEdge(2, 3);
            g->addEdge(3, 4);
            g->addEdge(4, 2);
            test_all_strategies(*g, -1);
        }

        SECTION("Path or Line graph is not strongly connected") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(4);
            g->addEdge(0, 1);
            g->addEdge(1, 2);
            g->addEdge(2, 3);
            test_all_strategies(*g, -1);
        }
    }

    SECTION("Strongly Connected Graphs") {
        SECTION("Two vertices with edges in both directions") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(2);
            g->addEdge(0, 1);
            g->addEdge(1, 0);
            test_all_strategies(*g, 1);
        }

        SECTION("Simple Cycle graph") {
            int n = 5;
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(n);
            for (int i = 0; i < n; ++i) {
                g->addEdge(i, (i + 1) % n);
            }
            test_all_strategies(*g, n - 1);
        }

        SECTION("Complete Directed Graph (Clique)") {
            int n = 5;
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(n);
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    if (i != j) {
                        g->addEdge(i, j);
                    }
                }
            }
            test_all_strategies(*g, 1);
        }

        SECTION("Wheel Graph (Strongly Connected Version)") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(4);
            // Hub (0) to cycle and back
            g->addEdge(0, 1); g->addEdge(1, 0);
            g->addEdge(0, 2); g->addEdge(2, 0);
            g->addEdge(0, 3); g->addEdge(3, 0);
            // Outer cycle
            g->addEdge(1, 2);
            g->addEdge(2, 3);
            g->addEdge(3, 1);
            test_all_strategies(*g, 2);
        }

        SECTION("A more complex, asymmetrical strongly connected graph") {
            IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(5);
            g->addEdge(0, 1); g->addEdge(1, 0); // 0 <-> 1
            g->addEdge(1, 2);
            g->addEdge(2, 3);
            g->addEdge(3, 4);
            g->addEdge(4, 1); // Cycle 1-2-3-4
            g->addEdge(0, 3); // Shortcut
            g->addEdge(4, 0); // Path back to 0
            test_all_strategies(*g, 3);
        }
    }

    SECTION("Randomized graph checks") {
        constexpr int num_tests = 20;
        constexpr int num_vertices = 10;
        constexpr int min_edges = 6;
        constexpr int max_edges = 200;

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

            // naive implementation source of truth
            const auto expected_result = std::get<int>(seq_strategy.execute(*g));

            const auto async_result = std::get<int>(async_strategy.execute(*g));
            const auto par_result = std::get<int>(par_strategy.execute(*g));

            INFO("Test run " << i + 1 << " with " << num_vertices << " vertices and " << num_edges << " edges.");
            REQUIRE(async_result == expected_result);
            REQUIRE(par_result == expected_result);
        }
    }
}