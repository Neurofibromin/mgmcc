module;

#include <iostream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <span>
#include <type_traits>

export module GraphNList;

import IGraph;
import GraphConcepts;

export class GraphNList : public IGraph {
private:
    int V;
    int E;
    std::vector<std::vector<int>> adj;
    std::vector<std::vector<int>> rev_adj;
public:
    using is_cache_local = std::false_type;
    using is_easily_mutable = std::true_type;

    explicit GraphNList(int num_vertices) : IGraph(), V(num_vertices), E(0) {
        if (num_vertices < 0) {
            throw std::invalid_argument("The number of verteces cannot be negative.");
        }
        adj.resize(V);
        rev_adj.resize(V);
    }

    template <IsGraph G>
    explicit GraphNList(const G& source_graph) : V(source_graph.numVertices()), E(source_graph.numEdges()) {
        if (V < 0) {
            throw std::invalid_argument("The number of a vertex cannot be negative.");
        }
        adj.resize(V);
        rev_adj.resize(V);

        for (int i = 0; i < V; ++i) {
            auto out_neighbors = source_graph.outneighbors(i);
            adj[i].assign(out_neighbors.begin(), out_neighbors.end());

            auto in_neighbors = source_graph.inneighbors(i);
            rev_adj[i].assign(in_neighbors.begin(), in_neighbors.end());
        }
    }

    GraphNList(const GraphNList &) = default;
    GraphNList(GraphNList &&) = default;
    GraphNList &operator=(const GraphNList &) = default;
    GraphNList &operator=(GraphNList &&) = default;

    int numVertices() const override {
        return V;
    }

    int numEdges() const override {
        return E;
    }

    void addEdge(int u, int v) override {
        if (u < 0 || u >= V || v < 0 || v >= V) {
            throw std::out_of_range("Invalid vertex index.");
        }
        adj[u].push_back(v);
        rev_adj[v].push_back(u);
        E++;
    }

    void removeEdge(int u, int v) override {
        if (u < 0 || u >= V || v < 0 || v >= V) {
            throw std::out_of_range("Invalid vertex index.");
        }
        auto& out_edges = adj[u];
        if (auto it = std::ranges::find(out_edges, v); it != out_edges.end()) {
            out_edges.erase(it);
            E--;

            auto& in_edges = rev_adj[v];
            if (auto it_in = std::ranges::find(in_edges, u); it_in != in_edges.end()) {
                in_edges.erase(it_in);
            }
        }
    }

    std::span<const int> outneighbors(int u) const override {
        if (u < 0 || u >= V) {
            throw std::out_of_range("Invalid vertex index.");
        }
        return adj[u];
    }

    std::span<const int> inneighbors(int u) const override {
        if (u < 0 || u >= V) {
            throw std::out_of_range("Invalid vertex index.");
        }
        return rev_adj[u];
    }

    int out_degree(int u) const override {
        if (u < 0 || u >= V) { throw std::out_of_range("Invalid vertex index."); }
        return adj[u].size();
    }

    int in_degree(int u) const override {
        if (u < 0 || u >= V) { throw std::out_of_range("Invalid vertex index."); }
        return rev_adj[u].size();
    }

    // GraphNList getTranspose() const {
    //     GraphNList g_t(V);
    //     for (int u = 0; u < V; ++u) {
    //         for (int v : this->adj[u]) {
    //             g_t.addEdge(v, u); //args order correct
    //         }
    //     }
    //     return g_t;
    // }

    GraphNList getTranspose() const {
        GraphNList g_t(V);
        g_t.adj = this->rev_adj;
        g_t.rev_adj = this->adj;
        g_t.E = this->E;
        return g_t;
    }
};