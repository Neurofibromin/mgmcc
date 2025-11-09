#include <catch2/catch_template_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <vector>
#include <utility>
#include <algorithm>
#include <memory>
#include <variant>
#include <functional>

import ImplementedGraph;
import GraphNList;
import GraphFList;
import GraphAMatrix;
import GraphFactory;
import GraphAlgo;
import AlgorithmResult;
import IAlgorithm;

bool is_dag_after_removal(const ImplementedGraph& original_graph, const std::vector<std::pair<int, int>>& fas) {
    auto g_copy = original_graph;
    for (const auto& edge : fas) {
        g_copy.removeEdge(edge.first, edge.second);
    }

    enum class Color { WHITE, GRAY, BLACK };
    std::vector<Color> colors(g_copy.numVertices(), Color::WHITE);

    std::function<bool(int)> has_cycle_util =
        [&](int u) -> bool {
        colors[u] = Color::GRAY;
        for (int v : g_copy.outneighbors(u)) {
            if (colors[v] == Color::GRAY) return true;
            if (colors[v] == Color::WHITE && has_cycle_util(v)) return true;
        }
        colors[u] = Color::BLACK;
        return false;
    };

    for (int i = 0; i < g_copy.numVertices(); ++i) {
        if (colors[i] == Color::WHITE) {
            if (has_cycle_util(i)) return false;
        }
    }
    return true;
}


TEMPLATE_TEST_CASE("Feedback Arc Set Algorithm", "[feedback_arc_set]", GraphNList, GraphFList, GraphAMatrix) {
    using IGraphPtr = std::unique_ptr<ImplementedGraph>;
    using GraphType = TestType;
    using FAS = std::vector<std::pair<int, int>>;

    // Instantiate all strategies
    GraphProcessor<ImplementedGraph>::FeedbackArcSetRemoveCyclesStrategy remove_cycles_strategy;
    GraphProcessor<ImplementedGraph>::FeedbackArcSetInsertEdgesStrategy insert_edges_strategy;
    GraphProcessor<ImplementedGraph>::FeedbackArcSetDfsStrategy dfs_strategy;

    auto test_strategy = [&](const IAlgorithm<ImplementedGraph>& strategy, const ImplementedGraph& g, const std::function<void(const FAS&)>& validation) {
        auto result = strategy.execute(g);
        const auto& fas = std::get<FAS>(result);
        INFO("Testing strategy: " << strategy.getName());
        REQUIRE(is_dag_after_removal(g, fas));
        validation(fas);
    };

    SECTION("Graph is already a DAG") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(4);
        g->addEdge(0, 1);
        g->addEdge(0, 2);
        g->addEdge(1, 3);

        auto validation = [](const FAS& fas) {
            REQUIRE(fas.empty());
        };

        test_strategy(remove_cycles_strategy, *g, validation);
        test_strategy(insert_edges_strategy, *g, validation);
        test_strategy(dfs_strategy, *g, validation);
    }

    SECTION("Single vertex with self-loop") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(1);
        g->addEdge(0, 0);

        auto validation = [](const FAS& fas) {
            REQUIRE_THAT(fas, Catch::Matchers::UnorderedEquals(FAS{{0, 0}}));
        };

        test_strategy(remove_cycles_strategy, *g, validation);
        test_strategy(insert_edges_strategy, *g, validation);
        test_strategy(dfs_strategy, *g, validation);
    }

    SECTION("Simple 3-vertex cycle") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(3);
        g->addEdge(0, 1);
        g->addEdge(1, 2);
        g->addEdge(2, 0);

        auto validation = [](const FAS& fas) {
            REQUIRE(fas.size() == 1);
            bool is_valid_edge = (fas[0] == std::make_pair(0,1) ||
                                  fas[0] == std::make_pair(1,2) ||
                                  fas[0] == std::make_pair(2,0));
            REQUIRE(is_valid_edge);
        };

        test_strategy(remove_cycles_strategy, *g, validation);
        test_strategy(insert_edges_strategy, *g, validation);
        test_strategy(dfs_strategy, *g, validation);
    }

    SECTION("Two disjoint cycles") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(6);
        g->addEdge(0, 1); g->addEdge(1, 2); g->addEdge(2, 0); // Cycle 1
        g->addEdge(3, 4); g->addEdge(4, 5); g->addEdge(5, 3); // Cycle 2

        auto validation = [](const FAS& fas) {
             REQUIRE(fas.size() == 2);
        };

        test_strategy(remove_cycles_strategy, *g, validation);
        test_strategy(insert_edges_strategy, *g, validation);
        test_strategy(dfs_strategy, *g, validation);
    }

    SECTION("Graph with two overlapping cycles (figure-eight)") {
        // 0 -> 1 -> 2 -> 0  and  2 -> 3 -> 4 -> 2
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(5);
        g->addEdge(0, 1);
        g->addEdge(1, 2);
        g->addEdge(2, 0);
        g->addEdge(2, 3);
        g->addEdge(3, 4);
        g->addEdge(4, 2);

        auto validation = [](const FAS& fas) {
            REQUIRE(fas.size() == 2);
        };

        test_strategy(remove_cycles_strategy, *g, validation);
        test_strategy(insert_edges_strategy, *g, validation);
        test_strategy(dfs_strategy, *g, validation);
    }

    SECTION("Complete Directed Graph (K4)") {
        int n = 4;
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(n);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i != j) {
                    g->addEdge(i, j);
                }
            }
        }

        auto validation = [](const FAS& fas) {
            REQUIRE(fas.size() > 0);
            REQUIRE(fas.size() <= 6);
        };

        test_strategy(remove_cycles_strategy, *g, validation);
        test_strategy(insert_edges_strategy, *g, validation);
        test_strategy(dfs_strategy, *g, validation);
    }
}