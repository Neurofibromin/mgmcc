#include <catch2/catch_template_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <vector>
#include <algorithm>
#include <memory>
#include <iostream>
#include <variant>

import ImplementedGraph;
import GraphNList;
import GraphFList;
import GraphAMatrix;
import GraphFactory;
import GraphAlgo;
import AlgorithmResult;

bool verify_sources(const GraphNList& graph, const std::vector<int>& reported_sources) {
    std::vector<bool> is_reported_source(graph.numVertices(), false);
    for (int s : reported_sources) {
        if (s < 0 || s >= graph.numVertices()) {
            std::cerr << "Error: source_vertexes returned an invalid vertex: " << s << std::endl;
            return false;
        }
        if (is_reported_source[s]) {
            std::cerr << "Error: source_vertexes returned a duplicate vertex: " << s << std::endl;
            return false;
        }
        is_reported_source[s] = true;
    }

    for (int i = 0; i < graph.numVertices(); ++i) {
        bool is_true_source = graph.inneighbors(i).empty();
        if (is_true_source != is_reported_source[i]) {
            if (is_true_source) {
                std::cerr << "Error: Vertex " << i << " is a source but was not reported." << std::endl;
            } else {
                std::cerr << "Error: Vertex " << i << " is not a source but was reported." << std::endl;
            }
            return false;
        }
    }
    return true;
}


TEMPLATE_TEST_CASE("Source Vertexes Algorithm", "[source_vertexes]", GraphNList, GraphFList, GraphAMatrix) {
    using IGraphPtr = std::unique_ptr<ImplementedGraph>;
    using GraphType = TestType;
    GraphProcessor<ImplementedGraph>::SourceVertexStrategy strategy;

    SECTION("Empty graph") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(0);
        auto result = strategy.execute(*g);
        const auto& sources = std::get<std::vector<int>>(result);
        REQUIRE(sources.empty());
    }

    SECTION("Graph with a single vertex") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(1);
        auto result = strategy.execute(*g);
        const auto& sources = std::get<std::vector<int>>(result);
        REQUIRE(sources == std::vector<int>{0});
    }

    SECTION("Graph with no edges") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(5);
        auto result = strategy.execute(*g);
        const auto& sources = std::get<std::vector<int>>(result);
        std::vector<int> expected = {0, 1, 2, 3, 4};
        REQUIRE_THAT(sources, Catch::Matchers::UnorderedEquals(expected));
    }

    SECTION("Simple DAG with one source") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(4);
        g->addEdge(0, 1);
        g->addEdge(0, 2);
        g->addEdge(1, 3);
        g->addEdge(2, 3);
        auto result = strategy.execute(*g);
        const auto& sources = std::get<std::vector<int>>(result);
        REQUIRE(sources == std::vector<int>{0});
    }

    SECTION("Line or Chain graph") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(4);
        g->addEdge(0, 1);
        g->addEdge(1, 2);
        g->addEdge(2, 3);
        auto result = strategy.execute(*g);
        const auto& sources = std::get<std::vector<int>>(result);
        REQUIRE(sources == std::vector<int>{0});
    }

    SECTION("Graph with multiple sources") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(5);
        g->addEdge(0, 2);
        g->addEdge(1, 2);
        g->addEdge(2, 3);
        g->addEdge(4, 3);
        auto result = strategy.execute(*g);
        const auto& sources = std::get<std::vector<int>>(result);
        std::vector<int> expected = {0, 1, 4};
        REQUIRE_THAT(sources, Catch::Matchers::UnorderedEquals(expected));
    }

    SECTION("Graph with a cycle and no sources") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(3);
        g->addEdge(0, 1);
        g->addEdge(1, 2);
        g->addEdge(2, 0);
        auto result = strategy.execute(*g);
        const auto& sources = std::get<std::vector<int>>(result);
        REQUIRE(sources.empty());
    }

    SECTION("Graph with a cycle and one source") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(4);
        g->addEdge(0, 1); // Source
        g->addEdge(1, 2);
        g->addEdge(2, 3);
        g->addEdge(3, 1); // Cycle 1-2-3
        auto result = strategy.execute(*g);
        const auto& sources = std::get<std::vector<int>>(result);
        REQUIRE(sources == std::vector<int>{0});
    }

    SECTION("Vertex with self-loop is not a source") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(3);
        g->addEdge(0, 0); // 0 has an incoming edge from itself.
        g->addEdge(1, 2); // 1 is a source.
        auto result = strategy.execute(*g);
        const auto& sources = std::get<std::vector<int>>(result);
        REQUIRE(sources == std::vector<int>{1});
    }

    SECTION("Complete graph has no sources") {
        int n = 4;
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(n);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i != j) {
                    g->addEdge(i, j);
                }
            }
        }
        auto result = strategy.execute(*g);
        const auto& sources = std::get<std::vector<int>>(result);
        REQUIRE(sources.empty());
    }

    SECTION("Disconnected graph") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(6);
        // Component 1 (source is 0)
        g->addEdge(0, 1);
        g->addEdge(1, 2);
        // Component 2 (source is 3)
        g->addEdge(3, 4);
        // Component 3 (isolated vertex 5 is a source)
        auto result = strategy.execute(*g);
        const auto& sources = std::get<std::vector<int>>(result);
        std::vector<int> expected = {0, 3, 5};
        REQUIRE_THAT(sources, Catch::Matchers::UnorderedEquals(expected));
    }

    SECTION("More complex graph with mixed features") {
        IGraphPtr g = GraphFactory<ImplementedGraph>::createGraph<GraphType>(7);
        // Component 1: 0 -> 1 -> 2 -> 0 (cycle, no sources within)
        g->addEdge(0, 1);
        g->addEdge(1, 2);
        g->addEdge(2, 0);

        // Component 2: 3 -> 4 (source is 3)
        g->addEdge(3, 4);

        // Component 3: 5 (isolated vertex, is a source)

        // A new source (6) that points into the cycle
        g->addEdge(6, 1);

        auto result = strategy.execute(*g);
        const auto& sources = std::get<std::vector<int>>(result);
        std::vector<int> expected = {3, 5, 6};
        REQUIRE_THAT(sources, Catch::Matchers::UnorderedEquals(expected));
    }
}