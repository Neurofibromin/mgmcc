module;

#include <vector>
#include <stdexcept>
#include <span>
#include <algorithm>
#include <ranges>
#include <generator>
#include <mutex>
#include <memory>

export module GraphAMatrix;

import IGraph;
import GraphConcepts;
import SpanView;

export class GraphAMatrix : public IGraph {
private:
    int V;
    int E;
    // A single flat vector for the adjacency matrix for better cache locality, stores u->v edge count
    std::vector<int> adj;
    std::vector<int> rev_adj; //transposed
    std::vector<int> out_degree_counts;
    std::vector<int> in_degree_counts;

    mutable std::vector<std::vector<int>> out_neighbor_cache;
    mutable std::vector<bool> out_cache_valid;
    mutable std::vector<std::vector<int>> in_neighbor_cache;
    mutable std::vector<bool> in_cache_valid;

    // thread-safe cache access
    mutable std::vector<std::unique_ptr<std::mutex>> out_neighbor_mutexes;
    mutable std::vector<std::unique_ptr<std::mutex>> in_neighbor_mutexes;


    [[nodiscard]] constexpr size_t get_index(int u, int v) const {
        return static_cast<size_t>(u) * V + v;
    }

    std::generator<const int> _generate_outneighbors(int u) const {
        for (int i = 0; i < V; ++i) {
            for (int j = 0; j < adj[get_index(u, i)]; ++j) {
                co_yield i;
            }
        }
    }

    std::generator<const int> _generate_inneighbors(int u) const {
        for (int i = 0; i < V; ++i) {
            for (int j = 0; j < rev_adj[get_index(u, i)]; ++j) {
                co_yield i;
            }
        }
    }

    void initialize_mutexes() {
        out_neighbor_mutexes.clear();
        in_neighbor_mutexes.clear();
        out_neighbor_mutexes.reserve(V);
        in_neighbor_mutexes.reserve(V);

        for (int i = 0; i < V; ++i) {
            out_neighbor_mutexes.push_back(std::make_unique<std::mutex>());
            in_neighbor_mutexes.push_back(std::make_unique<std::mutex>());
        }
    }

public:
    using is_cache_local = std::true_type;
    using is_easily_mutable = std::true_type;

    explicit GraphAMatrix(int num_vertices) : IGraph(), V(num_vertices), E(0) {
        if (num_vertices < 0) {
            throw std::invalid_argument("The number of vertices cannot be negative.");
        }
        if (V > 0) {
            adj.resize(static_cast<size_t>(V) * V, 0);
            rev_adj.resize(static_cast<size_t>(V) * V, 0);
            out_degree_counts.resize(V, 0);
            in_degree_counts.resize(V, 0);

            out_neighbor_cache.resize(V);
            out_cache_valid.resize(V, false);
            in_neighbor_cache.resize(V);
            in_cache_valid.resize(V, false);

            initialize_mutexes();
        }
    }

    template <IsGraph G>
    explicit GraphAMatrix(const G& source_graph) : V(source_graph.numVertices()), E(0) {
        if (V < 0) {
            throw std::invalid_argument("The number of vertices cannot be negative.");
        }
        if (V > 0) {
            adj.resize(static_cast<size_t>(V) * V, 0);
            rev_adj.resize(static_cast<size_t>(V) * V, 0);
            out_degree_counts.resize(V, 0);
            in_degree_counts.resize(V, 0);

            out_neighbor_cache.resize(V);
            out_cache_valid.resize(V, false);
            in_neighbor_cache.resize(V);
            in_cache_valid.resize(V, false);

            initialize_mutexes();
        }

        for (int u = 0; u < V; ++u) {
            for (int v : source_graph.outneighbors(u)) {
                if (v >= 0 && v < V) {
                    adj[get_index(u, v)]++;
                    ++out_degree_counts[u];
                    rev_adj[get_index(v, u)]++;
                    ++in_degree_counts[v];
                    E++;
                }
            }
        }
    }

    // copy ctor
    GraphAMatrix(const GraphAMatrix& other)
        : V(other.V), E(other.E),
          adj(other.adj), rev_adj(other.rev_adj),
          out_degree_counts(other.out_degree_counts),
          in_degree_counts(other.in_degree_counts),
          out_neighbor_cache(other.out_neighbor_cache),
          out_cache_valid(other.out_cache_valid),
          in_neighbor_cache(other.in_neighbor_cache),
          in_cache_valid(other.in_cache_valid) {
        if (V > 0) {
            initialize_mutexes();
        }
    }

    // move ctor
    GraphAMatrix(GraphAMatrix&& other) noexcept
        : V(other.V), E(other.E),
          adj(std::move(other.adj)), rev_adj(std::move(other.rev_adj)),
          out_degree_counts(std::move(other.out_degree_counts)),
          in_degree_counts(std::move(other.in_degree_counts)),
          out_neighbor_cache(std::move(other.out_neighbor_cache)),
          out_cache_valid(std::move(other.out_cache_valid)),
          in_neighbor_cache(std::move(other.in_neighbor_cache)),
          in_cache_valid(std::move(other.in_cache_valid)),
          out_neighbor_mutexes(std::move(other.out_neighbor_mutexes)),
          in_neighbor_mutexes(std::move(other.in_neighbor_mutexes)) {
        other.V = 0;
        other.E = 0;
    }

    // copy assignment
    GraphAMatrix& operator=(const GraphAMatrix& other) {
        if (this != &other) {
            V = other.V;
            E = other.E;
            adj = other.adj;
            rev_adj = other.rev_adj;
            out_degree_counts = other.out_degree_counts;
            in_degree_counts = other.in_degree_counts;
            out_neighbor_cache = other.out_neighbor_cache;
            out_cache_valid = other.out_cache_valid;
            in_neighbor_cache = other.in_neighbor_cache;
            in_cache_valid = other.in_cache_valid;

            if (V > 0) {
                initialize_mutexes();
            }
        }
        return *this;
    }

    // move assignment
    GraphAMatrix& operator=(GraphAMatrix&& other) noexcept {
        if (this != &other) {
            V = other.V;
            E = other.E;
            adj = std::move(other.adj);
            rev_adj = std::move(other.rev_adj);
            out_degree_counts = std::move(other.out_degree_counts);
            in_degree_counts = std::move(other.in_degree_counts);
            out_neighbor_cache = std::move(other.out_neighbor_cache);
            out_cache_valid = std::move(other.out_cache_valid);
            in_neighbor_cache = std::move(other.in_neighbor_cache);
            in_cache_valid = std::move(other.in_cache_valid);
            out_neighbor_mutexes = std::move(other.out_neighbor_mutexes);
            in_neighbor_mutexes = std::move(other.in_neighbor_mutexes);

            other.V = 0;
            other.E = 0;
        }
        return *this;
    }

    int numVertices() const override {
        return V;
    }

    int numEdges() const override {
        return E;
    }

    void addEdge(int u, int v) override {
        if (u < 0 || u >= V || v < 0 || v >= V) [[unlikely]] {
            throw std::out_of_range("Invalid vertex index.");
        }
        adj[get_index(u, v)]++;
        ++out_degree_counts[u];
        rev_adj[get_index(v, u)]++;
        ++in_degree_counts[v];
        E++;

        out_cache_valid[u] = false;
        in_cache_valid[v] = false;
    }

    void removeEdge(int u, int v) override {
        if (u < 0 || u >= V || v < 0 || v >= V) [[unlikely]] {
            throw std::out_of_range("Invalid vertex index.");
        }
        if (adj[get_index(u, v)] > 0) {
            adj[get_index(u, v)]--;
            --out_degree_counts[u];
            rev_adj[get_index(v, u)]--;
            --in_degree_counts[v];
            E--;

            out_cache_valid[u] = false;
            in_cache_valid[v] = false;
        }
    }

    std::span<const int> outneighbors(int u) const override {
        if (u < 0 || u >= V) {
            throw std::out_of_range("Invalid vertex index.");
        }
        // lock mutex for this vertex
        std::lock_guard<std::mutex> lock(*out_neighbor_mutexes[u]);
        if (!out_cache_valid[u]) {
            out_neighbor_cache[u].clear();
            out_neighbor_cache[u].reserve(out_degree_counts[u]);
            for (int v = 0; v < V; ++v) {
                for (int count = 0; count < adj[get_index(u, v)]; ++count) {
                    out_neighbor_cache[u].push_back(v);
                }
            }
            out_cache_valid[u] = true;
        }
        return out_neighbor_cache[u];
    }

    std::span<const int> inneighbors(int u) const override {
        if (u < 0 || u >= V) {
            throw std::out_of_range("Invalid vertex index.");
        }
        // lock mutex for this vertex
        std::lock_guard<std::mutex> lock(*in_neighbor_mutexes[u]);
        if (!in_cache_valid[u]) {
            in_neighbor_cache[u].clear();
            in_neighbor_cache[u].reserve(in_degree_counts[u]);
            for (int v = 0; v < V; ++v) {
                for (int count = 0; count < rev_adj[get_index(u, v)]; ++count) {
                    in_neighbor_cache[u].push_back(v);
                }
            }
            in_cache_valid[u] = true;
        }
        return in_neighbor_cache[u];
    }

    std::span<const int> connectedto(int u) const {
        if (u < 0 || u >= V) {
            throw std::out_of_range("Invalid vertex index.");
        }
        return std::span<const int>(&adj[get_index(u, 0)], V);
    }

    int out_degree(int u) const override {
        if (u < 0 || u >= V) { throw std::out_of_range("Invalid vertex index."); }
        return out_degree_counts[u];
    }

    int in_degree(int u) const override {
        if (u < 0 || u >= V) { throw std::out_of_range("Invalid vertex index."); }
        return in_degree_counts[u];
    }

    GraphAMatrix getTranspose() const {
        GraphAMatrix g_t(V);
        g_t.adj = this->rev_adj;
        g_t.rev_adj = this->adj;
        g_t.out_degree_counts = this->in_degree_counts;
        g_t.in_degree_counts = this->out_degree_counts;
        g_t.E = this->E;
        return g_t;
    }
};