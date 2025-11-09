module;

#include <iostream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <span>
#include <numeric>
#include <ranges>
#include <execution>

export module GraphFList;

import IGraph;
import GraphConcepts;

export class GraphFList : public IGraph {
private:
    int V;
    // out-edges
    std::vector<int> edge_targets;
    std::vector<int> out_offsets;

    // in-edges
    std::vector<int> rev_edge_targets;
    std::vector<int> in_offsets;

public:
    using is_cache_local = std::true_type;
    using is_easily_mutable = std::false_type;
    explicit GraphFList(int num_vertices) : V(num_vertices) {
        if (num_vertices < 0) {
            throw std::invalid_argument("The number of vertices cannot be negative.");
        }
        out_offsets.resize(V + 1, 0);
        in_offsets.resize(V + 1, 0);
    }

    template <IsGraph G>
    explicit GraphFList(const G& source_graph) : V(source_graph.numVertices()) {
        out_offsets.resize(V + 1);
        in_offsets.resize(V + 1);

        if (V == 0) {
            out_offsets[V] = 0;
            in_offsets[V] = 0;
            return;
        }

        auto vertex_indices = std::views::iota(0, V);
        size_t total_edges = std::transform_reduce(
            std::execution::par,
            vertex_indices.begin(),
            vertex_indices.end(),
            static_cast<size_t>(0),
            std::plus<>(),          // Reduce
            [&](int i) {            // Transform
                return source_graph.outneighbors(i).size();
            }
        );

        edge_targets.reserve(total_edges);
        rev_edge_targets.reserve(total_edges);

        // Build forward
        for (int i = 0; i < V; ++i) {
            out_offsets[i] = edge_targets.size();
            auto neighbors = source_graph.outneighbors(i);
            edge_targets.insert(edge_targets.end(), neighbors.begin(), neighbors.end());
        }
        out_offsets[V] = edge_targets.size();

        // Build reverse
        for (int i = 0; i < V; ++i) {
            in_offsets[i] = rev_edge_targets.size();
            auto neighbors = source_graph.inneighbors(i);
            rev_edge_targets.insert(rev_edge_targets.end(), neighbors.begin(), neighbors.end());
        }
        in_offsets[V] = rev_edge_targets.size();
    }


    GraphFList(const GraphFList &) = default;
    GraphFList(GraphFList &&) = default;
    GraphFList &operator=(const GraphFList &) = default;
    GraphFList &operator=(GraphFList &&) = default;

    auto numVertices() const -> int override {
        return V;
    }

    auto numEdges() const -> int override {
        return edge_targets.size();
    }

    auto addEdge(int u, int v) -> void override {
        if (u < 0 || u >= V || v < 0 || v >= V) {
            throw std::out_of_range("Invalid vertex index.");
        }

        // Insert 'v' at the end of u's neighbour list
        auto insert_pos_out = edge_targets.begin() + out_offsets[u+1];
        edge_targets.insert(insert_pos_out, v); //SLOW

        // Update following offsets
        for (int i = u + 1; i <= V; ++i) {
            out_offsets[i]++;
        }

        // reverse edge
        auto insert_pos_in = rev_edge_targets.begin() + in_offsets[v+1];
        rev_edge_targets.insert(insert_pos_in, u);
        for (int i = v + 1; i <= V; ++i) {
            in_offsets[i]++;
        }
    }

    auto removeEdge(int u, int v) -> void override {
        if (u < 0 || u >= V || v < 0 || v >= V) {
            throw std::out_of_range("Invalid vertex index.");
        }

        //  forward edge
        auto out_start_it = edge_targets.begin() + out_offsets[u];
        auto out_end_it = edge_targets.begin() + out_offsets[u+1];
        auto it_out = std::find(out_start_it, out_end_it, v);

        if (it_out != out_end_it) {
            edge_targets.erase(it_out);
            for (int i = u + 1; i <= V; ++i) {
                out_offsets[i]--;
            }
        }

        //  reverse
        auto in_start_it = rev_edge_targets.begin() + in_offsets[v];
        auto in_end_it = rev_edge_targets.begin() + in_offsets[v+1];
        auto it_in = std::find(in_start_it, in_end_it, u);

        if (it_in != in_end_it) {
            rev_edge_targets.erase(it_in);
            for (int i = v + 1; i <= V; ++i) {
                in_offsets[i]--;
            }
        }
    }

    std::span<const int> outneighbors(int u) const override {
        if (u < 0 || u >= V) {
            throw std::out_of_range("Invalid vertex index.");
        }
        const auto start_idx = out_offsets[u];
        const auto count = out_offsets[u+1] - start_idx;
        if (count == 0) return {};
        return {&edge_targets[start_idx], static_cast<size_t>(count)};
    }

    std::span<const int> inneighbors(int u) const override {
        if (u < 0 || u >= V) {
            throw std::out_of_range("Invalid vertex index.");
        }
        const auto start_idx = in_offsets[u];
        const auto count = in_offsets[u+1] - start_idx;
        if (count == 0) return {};
        return {&rev_edge_targets[start_idx], static_cast<size_t>(count)};
    }

    auto out_degree(int u) const -> int override {
        if (u < 0 || u >= V) { throw std::out_of_range("Invalid vertex index."); }
        return out_offsets[u+1] - out_offsets[u];
    }

    auto in_degree(int u) const -> int override {
        if (u < 0 || u >= V) { throw std::out_of_range("Invalid vertex index."); }
        return in_offsets[u+1] - in_offsets[u];
    }

    GraphFList getTranspose() const {
        GraphFList g_t(V);
        g_t.edge_targets = this->rev_edge_targets;
        g_t.out_offsets = this->in_offsets;
        g_t.rev_edge_targets = this->edge_targets;
        g_t.in_offsets = this->out_offsets;
        return g_t;
    }
};