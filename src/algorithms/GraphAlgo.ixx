module;

#include <algorithm>
#include <atomic>
#include <execution>
#include <future>
#include <iostream>
#include <numeric>
#include <optional>
#include <queue>
#include <ranges>
#include <utility>
#include <vector>
#include <stack>
#include <thread>

export module GraphAlgo;

import GraphConcepts;
import IAlgorithm;
import AlgorithmResult;
import Properties;

class ImplementedGraph;

struct BfsResult {
    int eccentricity;
    bool is_connected;
};

export template <IsGraph GraphTypeImplementationGeneralizer = ImplementedGraph, typename AlgorithmInterface = IAlgorithm<GraphTypeImplementationGeneralizer>, bool isDebugMode = false>
requires std::is_same_v<GraphTypeImplementationGeneralizer, typename AlgorithmInterface::implementation_generalizer_type>
class GraphProcessor
{
public:
    using implementation_generalizer_type = GraphTypeImplementationGeneralizer;
    using algorithm_interface_type = AlgorithmInterface;

    static void DFS_util_recursive(const GraphTypeImplementationGeneralizer& g, int u, std::vector<bool>& visited, std::vector<int>& finish_order) {
        visited[u] = true;
        for (int v : g.outneighbors(u)) {
            if (!visited[v]) {
                DFS_util_recursive(g, v, visited, finish_order);
            }
        }
        finish_order.push_back(u);
    }

    // Iterative version of DFS_util
    static void DFS_util(const GraphTypeImplementationGeneralizer& g, int u_start, std::vector<bool>& visited, std::vector<int>& finish_order) {
        auto initial_neighbors = g.outneighbors(u_start);
        using neighbor_iterator = decltype(initial_neighbors.begin());

        std::vector<std::pair<int, neighbor_iterator>> stack;

        stack.emplace_back(u_start, initial_neighbors.begin());
        visited[u_start] = true;

        while (!stack.empty()) {
            auto& [u, iter] = stack.back();
            auto end_iter = g.outneighbors(u).end();

            bool found_unvisited = false;
            while (iter != end_iter) {
                int v = *iter;
                ++iter;
                if (!visited[v]) {
                    visited[v] = true;
                    auto v_neighbors = g.outneighbors(v);
                    stack.emplace_back(v, v_neighbors.begin());
                    found_unvisited = true;
                    break;
                }
            }

            if (!found_unvisited) {
                finish_order.push_back(u);
                stack.pop_back();
            }
        }
    }

    static void DFS_collect_scc_recursive(const GraphTypeImplementationGeneralizer& g, int u, std::vector<bool>& visited, std::vector<int>& component) {
        //should use transposed
        visited[u] = true;
        component.push_back(u);
        for (int v : g.outneighbors(u)) {
            if (!visited[v]) {
                DFS_collect_scc_recursive(g, v, visited, component);
            }
        }
    }

    // Iterative version of DFS_collect_scc
    static void DFS_collect_scc(const GraphTypeImplementationGeneralizer& g, int u_start, std::vector<bool>& visited, std::vector<int>& component) {
        std::vector<int> stack;
        stack.push_back(u_start);
        visited[u_start] = true;

        while (!stack.empty()) {
            int u = stack.back();
            stack.pop_back();
            component.push_back(u);

            auto neighbors_range = g.outneighbors(u);
            std::vector<int> neighbors(std::begin(neighbors_range), std::end(neighbors_range));

            for (auto it = neighbors.rbegin(); it != neighbors.rend(); ++it) {
                int v = *it;
                if (!visited[v]) {
                    visited[v] = true;
                    stack.push_back(v);
                }
            }
        }
    }

    /**
     * @brief Checks if a path exists from a start node to an end node using BFS.
     */
    static bool has_path(const GraphTypeImplementationGeneralizer& g, int start_node, int end_node) {
        if (start_node == end_node) return true;
        std::vector<bool> visited(g.numVertices(), false);
        std::queue<int> q;

        q.push(start_node);
        visited[start_node] = true;

        while (!q.empty()) {
            int u = q.front();
            q.pop();

            for (int v : g.outneighbors(u)) {
                if (v == end_node) return true;
                if (!visited[v]) {
                    visited[v] = true;
                    q.push(v);
                }
            }
        }
        return false;
    }

    class SourceVertexStrategy : public AlgorithmInterface {
    public:
        using solves_problem = Problem::SourceVertexCount;
        using properties = AlgorithmProperties::NoPreference;
        using preferred_graph_properties = GraphProperties::CacheLocal;
        using algorithm_interface = AlgorithmInterface;
        const char* getName() const override { return "1"; }
        AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
            std::vector<int> sources;
            const int num_vertices = g.numVertices();
            sources.reserve(num_vertices / 10);
            for (int i = 0; i < num_vertices; ++i) {
                if (g.in_degree(i) == 0) {
                    sources.push_back(i);
                }
            }
            //could do something parallel, but probably slower:
            /*
            auto const vertices = std::views::iota(0, num_vertices);
            std::mutex mtx;

            std::for_each(std::execution::par, vertices.begin(), vertices.end(),
                [&](int i) {
                    if (g.inneighbors(i).empty()) {
                        std::lock_guard<std::mutex> lock(mtx);
                        sources.push_back(i);
                    }
                });
             *
             */
            return sources;
        }
    };

    class SequentialDiameterStrategy : public AlgorithmInterface {
        public:
        using solves_problem = Problem::DiameterMeasure;
        using properties = AlgorithmProperties::SparseGraphPreferred;
        using preferred_graph_properties = GraphProperties::CacheLocal;
        using algorithm_interface = AlgorithmInterface;
        const char* getName() const override { return "2-seq"; }
        AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
            const int num_vertices = g.numVertices();
            if (num_vertices <= 1) return 0;
            int max_diameter = 0;

            for (int i = 0; i < num_vertices; ++i) { //BFS from every vertex
                std::vector<int> dist(num_vertices, -1);
                std::queue<int> q;
                dist[i] = 0;
                q.push(i);
                int visited_count = 1;
                int current_max = 0;
                while (!q.empty()) {
                    int u = q.front();
                    q.pop();

                    for (int v : g.outneighbors(u)) { //BFS loop
                        if (dist[v] == -1) {
                            dist[v] = dist[u] + 1;
                            q.push(v);
                            visited_count++;
                            current_max = std::max(current_max, dist[v]);
                        }
                    }
                }

                // Early termination
                if (visited_count != num_vertices) {
                    return -1;
                }
                max_diameter = std::max(max_diameter, current_max);
            }
            return max_diameter;
        }
    };

    class AsyncDiameterStrategy : public AlgorithmInterface {
    public:
        using solves_problem = Problem::DiameterMeasure;
        using properties = AlgorithmProperties::SparseGraphPreferred;
        using preferred_graph_properties = GraphProperties::CacheLocal;
        using algorithm_interface = AlgorithmInterface;
        const char* getName() const override {
            if (isDebugMode) return "2-async";
            else return "2";
        }
        AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
            const int num_vertices = g.numVertices();
            if (num_vertices <= 1) return 0;

            const unsigned int num_cores = std::thread::hardware_concurrency();
            const int num_workers = (num_cores > 0) ? num_cores*2 : 4;
            const int workers_to_launch = std::min(num_vertices, num_workers);

            std::vector<std::future<std::vector<BfsResult>>> futures;
            futures.reserve(workers_to_launch);

            std::atomic<int> next_vertex_idx(0);

            // LAUNCH WORKERS
            for (int i = 0; i < workers_to_launch; ++i) {
                futures.push_back(std::async(std::launch::async, [&]() {
                    std::vector<BfsResult> local_results;
                    int vertex_idx;
                    // Worker loop
                    while ((vertex_idx = next_vertex_idx.fetch_add(1)) < num_vertices) {
                        std::vector<int> dist(num_vertices, -1);
                        std::queue<int> q;
                        dist[vertex_idx] = 0;
                        q.push(vertex_idx);
                        int visited_count = 1;
                        int current_max = 0;
                        while (!q.empty()) {
                            int u = q.front();
                            q.pop();
                            for (int v : g.outneighbors(u)) {
                                if (dist[v] == -1) {
                                    dist[v] = dist[u] + 1;
                                    q.push(v);
                                    visited_count++;
                                    current_max = std::max(current_max, dist[v]);
                                }
                            }
                        }
                        bool connected_from_this_node = (visited_count == num_vertices);
                        local_results.push_back(BfsResult{current_max, connected_from_this_node});
                    }
                    return local_results;
                }));
            }

            //AGGREGATION
            int max_diameter = 0;
            bool is_graph_strongly_connected = true;
            for (auto& fut : futures) {
                std::vector<BfsResult> local_results = fut.get();
                for (const auto& result : local_results) {
                    if (!result.is_connected) {
                        is_graph_strongly_connected = false;
                    }
                    if (is_graph_strongly_connected) {
                        max_diameter = std::max(max_diameter, result.eccentricity);
                    }
                }
            }

            if (!is_graph_strongly_connected) return -1;
            return max_diameter;
        }
    };

    class ParallelDiameterStrategy : public AlgorithmInterface {
    public:
        using solves_problem = Problem::DiameterMeasure;
        using properties = AlgorithmProperties::SparseGraphPreferred;
        using preferred_graph_properties = GraphProperties::CacheLocal;
        using algorithm_interface = AlgorithmInterface;
        const char* getName() const override {
            if (isDebugMode) return "2-par";
            else return "2";
        }
        AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
            const int num_vertices = g.numVertices();
            if (num_vertices <= 1) return 0;
            auto indices = std::views::iota(0, num_vertices);
            BfsResult initial_value = {0, true};
            BfsResult final_result = std::transform_reduce(
                std::execution::par, //maybe par_unseq also works?
                indices.begin(), indices.end(),
                initial_value,
                //Reducer
                [](BfsResult a, BfsResult b) {
                    return BfsResult {
                        std::max(a.eccentricity, b.eccentricity),
                        // branchless ?? a.is_connected && b.is_connected
                        static_cast<bool>(a.is_connected & b.is_connected)
                    };
                },
                //Transform
                [&g, num_vertices](int i) {
                    std::vector<int> dist(num_vertices, -1);
                    std::queue<int> q;
                    dist[i] = 0;
                    q.push(i);
                    int visited_count = 1;
                    int current_max = 0;
                    while (!q.empty()) {
                        int u = q.front();
                        q.pop();
                        for (int v : g.outneighbors(u)) {
                            if (dist[v] == -1) {
                                dist[v] = dist[u] + 1;
                                q.push(v);
                                visited_count++;
                                current_max = std::max(current_max, dist[v]);
                            }
                        }
                    }
                    return BfsResult{current_max, (visited_count == num_vertices)};
                }
            );

            // branchless: final_result.is_connected ? final_result.eccentricity : -1; false == 0
            return (final_result.eccentricity * final_result.is_connected) + (-1 * !final_result.is_connected);
        }
    };

    class FeedbackArcSetRemoveCyclesStrategy : public AlgorithmInterface {
    public:
        using solves_problem = Problem::FeedbackArcSet;
        using properties = AlgorithmProperties::SparseGraphPreferred;
        using preferred_graph_properties = GraphProperties::EasilyMutable;
        using algorithm_interface = AlgorithmInterface;
        const char* getName() const override {
            if (isDebugMode) return "3-remove_edges";
            else return "3a";
        }

        AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
            const int num_vertices = g.numVertices();
            if (num_vertices == 0) {
                return std::vector<std::pair<int, int>>{};
            }

            GraphTypeImplementationGeneralizer graph_copy {g};
            std::vector<std::pair<int, int>> removed_edges;

            // find cycles
            while (true) {
                std::optional<std::pair<int, int>> back_edge = find_cycle(graph_copy);
                if (back_edge) {
                    graph_copy.removeEdge(back_edge->first, back_edge->second);
                    removed_edges.push_back(*back_edge);
                } else {
                    break; // No more cycles
                }
            }

            // re-insert
            std::vector<std::pair<int, int>> minimal_feedback_arc_set;
            for (const auto& edge : removed_edges) {
                int u = edge.first;
                int v = edge.second;

                // If v->u path exists, re-adding (u, v) would create a cycle.
                if (has_path(graph_copy, v, u)) {
                    minimal_feedback_arc_set.push_back(edge);
                } else {
                    graph_copy.addEdge(u, v);
                }
            }

            return minimal_feedback_arc_set;
        }

        std::optional<std::pair<int, int>> find_cycle(const GraphTypeImplementationGeneralizer& g) const {
            const int num_vertices = g.numVertices();
            std::vector<bool> visited(num_vertices, false);
            // std::vector<bool> on_current_path(num_vertices, false);

            for (int i = 0; i < num_vertices; ++i) {
                if (!visited[i]) {
                    if (auto back_edge = find_cycle_dfs_util(g, i, visited/*, on_current_path*/); back_edge) {
                        return back_edge;
                    }
                }
            }
            return std::nullopt;
        }

        /**
         * @brief Finds a cycle in the graph using DFS and returns the back edge that forms it.
         */
        // std::optional<std::pair<int, int>> find_cycle_dfs_util_recursive(const GraphTypeImplementationGeneralizer& g, int u, std::vector<bool>& visited,
        //                                                       std::vector<bool>& on_current_path) const {
        //     visited[u] = true;
        //     on_current_path[u] = true;
        //
        //     for (int v : g.outneighbors(u)) {
        //         if (!visited[v]) [[likely]] {
        //             if (auto back_edge = find_cycle_dfs_util_recursive(g, v, visited, on_current_path); back_edge) {
        //                 return back_edge;
        //             }
        //         } else if (on_current_path[v]) [[unlikely]] {
        //             return std::make_pair(u, v);
        //         }
        //     }
        //     on_current_path[u] = false;
        //     return std::nullopt;
        // }

        // Iterative
        std::optional<std::pair<int, int>> find_cycle_dfs_util(const GraphTypeImplementationGeneralizer& g, int u_start, std::vector<bool>& visited) const {
            auto initial_neighbors = g.outneighbors(u_start);
            std::vector<int> stack;
            std::vector<bool> on_current_path(g.numVertices(), false);
            stack.emplace_back(u_start);
            visited[u_start] = true;
            on_current_path[u_start] = true;

            while (!stack.empty()) {
                auto& u = stack.back();
                auto iter = g.outneighbors(u).begin();
                auto end_iter = g.outneighbors(u).end();

                bool found_new_path = false;
                while (iter != end_iter) {
                    int v = *iter;
                    ++iter;

                    if (on_current_path[v]) {
                        return std::make_pair(u, v);
                    }

                    if (!visited[v]) {
                        visited[v] = true;
                        on_current_path[v] = true;
                        stack.emplace_back(v);
                        found_new_path = true;
                        break;
                    }
                }

                if (!found_new_path) {
                    on_current_path[stack.back()] = false;
                    stack.pop_back();
                }
            }
            return std::nullopt;
        }
    };

    class FeedbackArcSetInsertEdgesStrategy : public AlgorithmInterface {
    public:
        using solves_problem = Problem::FeedbackArcSet;
        using properties = AlgorithmProperties::SparseGraphPreferred;
        using preferred_graph_properties = GraphProperties::EasilyMutable;
        using algorithm_interface = AlgorithmInterface;
        const char* getName() const override {
            if (isDebugMode) return "3-insert_edges";
            else return "3b";
        }

        AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
            const int num_vertices = g.numVertices();
            if (num_vertices == 0) {
                return std::vector<std::pair<int, int>>{};
            }

            // Start with an empty graph of the same concrete type as g.
            GraphTypeImplementationGeneralizer acyclic_graph = std::visit(
                [num_vertices](const auto& concrete_graph) {
                    using ConcreteGraphType = std::decay_t<decltype(concrete_graph)>;
                    return GraphTypeImplementationGeneralizer(ConcreteGraphType(num_vertices));
                },
                g.getVariant());

            // This would also work: GraphTypeImplementationGeneralizer acyclic_graph {g};

            std::vector<std::pair<int, int>> all_edges;
            all_edges.reserve(g.numVertices()); //prealloc
            for (int u = 0; u < num_vertices; ++u) {
                for (int v : g.outneighbors(u)) {
                    all_edges.push_back({u, v});
                }
            }

            // Heuristic for ordering: process edges from vertices with a high (out-degree - in-degree) first.
            std::vector<long> delta(num_vertices);
            for (int i = 0; i < num_vertices; ++i) {
                delta[i] = static_cast<long>(g.outneighbors(i).size()) - g.inneighbors(i).size();
            }
            // could be operator<=> on a class of edges
            auto compare_edges = [&](const auto& edge_a, const auto& edge_b) {
                // Primary sort key: delta of source vertex, descending.
                long delta_u_a = delta[edge_a.first];
                long delta_u_b = delta[edge_b.first];
                if (delta_u_a != delta_u_b) {
                    return delta_u_a > delta_u_b;
                }
                // Secondary sort key: delta of target vertex, ascending.
                long delta_v_a = delta[edge_a.second];
                long delta_v_b = delta[edge_b.second];
                 if (delta_v_a != delta_v_b) {
                    return delta_v_a < delta_v_b;
                }
                // Tertiary key for a stable sort order
                if (edge_a.first != edge_b.first) {
                    return edge_a.first < edge_b.first;
                }
                return edge_a.second < edge_b.second;
            };
            std::ranges::sort(all_edges, compare_edges);
            std::vector<std::pair<int, int>> discarded_edges;
            for (const auto& edge : all_edges) {
                auto [u, v] = edge;
                // Check if adding the edge (u, v) would create a cycle.
                if (has_path(acyclic_graph, v, u)) {
                    discarded_edges.push_back(edge);
                } else {
                    acyclic_graph.addEdge(u, v);
                }
            }
            return discarded_edges;
        }
    };

    /**
     * @brief Finds a feedback arc set by identifying back edges in a DFS traversal.
     */
    class FeedbackArcSetDfsStrategy : public AlgorithmInterface {
    public:
        using solves_problem = Problem::FeedbackArcSet;
        using properties = AlgorithmProperties::DenseGraphPreferred;
        using preferred_graph_properties = GraphProperties::CacheLocal;
        using algorithm_interface = AlgorithmInterface;
        const char* getName() const override {
            if (isDebugMode) return "3-DFS";
            else return "3c";
        }

        AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
            const int num_vertices = g.numVertices();
            std::vector<std::pair<int, int>> back_edges;
            std::vector<Color> colors(num_vertices, Color::WHITE);

            for (int i = 0; i < num_vertices; ++i) {
                if (colors[i] == Color::WHITE) {
                    coloured_dfs_util_recursive(g, i, colors, back_edges);
                }
            }
            return back_edges;
        }

        enum class Color { WHITE, GRAY, BLACK };

        void coloured_dfs_util_recursive(const GraphTypeImplementationGeneralizer& g, int u, std::vector<Color>& colors, std::vector<std::pair<int, int>>& back_edges) const {
            colors[u] = Color::GRAY;

            for (int v : g.outneighbors(u)) {
                if (colors[v] == Color::GRAY) {
                    // This is a back edge, so cycle
                    back_edges.push_back({u, v});
                } else if (colors[v] == Color::WHITE) {
                    coloured_dfs_util_recursive(g, v, colors, back_edges);
                }
            }
            colors[u] = Color::BLACK;
        }

        // Iterative, but slower, as it checks every vertex 2x
        void coloured_dfs_util(const GraphTypeImplementationGeneralizer& g, int u_start, std::vector<Color>& colors, std::vector<std::pair<int, int>>& back_edges) const {
            std::vector<int> stack;
            stack.emplace_back(u_start);
            colors[u_start] = Color::GRAY;
            while (!stack.empty()) {
                auto& u = stack.back();
                auto iter = g.outneighbors(u).begin();
                auto end_iter = g.outneighbors(u).end();
                bool found_new_path = false;
                while (iter != end_iter) {
                    int v = *iter;
                    ++iter;

                    if (colors[v] == Color::GRAY) {
                        back_edges.push_back({u, v});
                    } else if (colors[v] == Color::WHITE) {
                        colors[v] = Color::GRAY;
                        stack.emplace_back(v);
                        found_new_path = true;
                        break;
                    }
                }
                if (!found_new_path) {
                    colors[stack.back()] = Color::BLACK;
                    stack.pop_back();
                }
            }
        }
    };

    /**
     * @brief Finds the first vertex from which all other vertices are reachable brute force
     */
    class SequentialUniversalSourceFinderStrategy : public AlgorithmInterface {
    public:
        using solves_problem = Problem::FirstUniversalSource;
        using properties = AlgorithmProperties::SparseGraphPreferred;
        using preferred_graph_properties = GraphProperties::CacheLocal;
        using algorithm_interface = AlgorithmInterface;
        const char* getName() const override { return "4-seq"; }
        [[deprecated("Very slow")]] AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
            const int num_vertices = g.numVertices();
            if (num_vertices == 0) return -1;
            if (num_vertices == 1) return 0;
            for (int i = 0; i < num_vertices; ++i) {
                std::vector<bool> visited(num_vertices, false);
                std::queue<int> q;
                int count = 0;
                q.push(i);
                visited[i] = true;
                count++;
                while (!q.empty()) {
                    int u = q.front();
                    q.pop();
                    for (int v : g.outneighbors(u)) {
                        if (!visited[v]) {
                            visited[v] = true;
                            q.push(v);
                            count++;
                        }
                    }
                }
                if (count == num_vertices) return i;
            }
            return -1;
        }
    };

    /**
     * @brief Finds the first vertex from which all other vertices are reachable Brute Force Parallel
     */
    class ParallelUniversalSourceFinderStrategy : public AlgorithmInterface {
    public:
        using solves_problem = Problem::FirstUniversalSource;
        using properties = AlgorithmProperties::SparseGraphPreferred;
        using preferred_graph_properties = GraphProperties::CacheLocal;
        using algorithm_interface = AlgorithmInterface;
        const char* getName() const override { return "4-par"; }
        AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
            const int num_vertices = g.numVertices();
        if (num_vertices == 0) {
            return -1;
        }
        if (num_vertices == 1) {
            return 0;
        }
        // Initialize with num_vertices, which is an invalid index and acts as "infinity".
        std::atomic<int> min_mother_vertex_idx(num_vertices);
        std::vector<int> indices(num_vertices);
        std::iota(indices.begin(), indices.end(), 0);

        std::for_each(
            std::execution::par,
            indices.begin(),
            indices.end(),
            [&](int i) {
                // Early exit for this vertex
                if (i >= min_mother_vertex_idx.load(std::memory_order_relaxed)) {
                    return;
                }
                std::vector<bool> visited(num_vertices, false);
                std::queue<int> q;
                q.push(i);
                visited[i] = true;
                int count = 1;
                while (!q.empty()) {
                    int u = q.front();
                    q.pop();
                    for (int v : g.outneighbors(u)) {
                        if (!visited[v]) {
                            visited[v] = true;
                            q.push(v);
                            count++;
                        }
                    }
                }
                //Atomic Update
                if (count == num_vertices) {
                    int expected = min_mother_vertex_idx.load();
                    while (i < expected) {
                        if (min_mother_vertex_idx.compare_exchange_weak(expected, i)) {
                            break; // Success
                        }
                    }
                }
            }
        );
        int result = min_mother_vertex_idx.load();
        return (result == num_vertices) ? -1 : result;
        }
    };

    class KosarajuUniversalSourceFinderStrategy : public AlgorithmInterface {
    public:
        using solves_problem = Problem::FirstUniversalSource;
        using properties = AlgorithmProperties::SparseGraphPreferred;
        using preferred_graph_properties = GraphProperties::CacheLocal;
        using algorithm_interface = AlgorithmInterface;
        const char* getName() const override {
            if (isDebugMode) return "4-Kosaraju";
            else return "4";
        }
        AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
            const int num_vertices = g.numVertices();
        if (num_vertices == 0) return -1;
        if (num_vertices == 1) return 0;
        // Kosaraju's First Pass
        std::vector<int> finish_order;
        finish_order.reserve(num_vertices);
        std::vector<bool> visited(num_vertices, false);
        for (int i = 0; i < num_vertices; ++i) {
            if (!visited[i]) {
                DFS_util(g, i, visited, finish_order);
            }
        }
        // Verify a mother vertex
        int candidate_vertex = finish_order.back();
        std::ranges::fill(visited, false);
        std::vector<int> reach_count_vec;
        DFS_util(g, candidate_vertex, visited, reach_count_vec);
        if (reach_count_vec.size() != num_vertices) {
            return -1; // No mother vertex
        }
        // Kosaraju's Second Pass - Find the source SCC ---
        // A mother vertex exists, and it must be in the source SCC.
        // The source SCC is the one containing the 'candidate' vertex.
        GraphTypeImplementationGeneralizer g_transpose = g.getTranspose();
        std::ranges::fill(visited, false);
        std::vector<std::vector<int>> scc_list;
        // Iterate the finish_order vector in reverse to process in the correct order
        for (const int v : std::views::reverse(finish_order)) {
            if (!visited[v]) {
                std::vector<int> current_scc;
                // The DFS for collecting SCCs must be on the TRANSPOSED graph
                DFS_collect_scc(g_transpose, v, visited, current_scc);
                scc_list.push_back(current_scc);
            }
        }
        // Find the source SCC and the minimum element within it
        // The source SCC is the one that contains the original candidate.
        for (const auto& scc : scc_list) {
            // Check if the candidate vertex is in the current SCC
            if (std::ranges::find(scc, candidate_vertex) != scc.end()) {
                // This is the source SCC. Find the smallest vertex in it.
                return *std::min_element(scc.begin(), scc.end());
            }
        }
        std::cerr << "Error: 273773f1-1f4f-48f1-b174-70184be5938a" << std::endl; // the candidate verification passed?
        return -1;
        }
    };

    class TarjanUniversalSourceFinderStrategy : public AlgorithmInterface {
        //based on: https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
    public:
        using solves_problem = Problem::FirstUniversalSource;
        using properties = AlgorithmProperties::SparseGraphPreferred;
        using preferred_graph_properties = GraphProperties::CacheLocal;
        using algorithm_interface = AlgorithmInterface;
        const char* getName() const override {
            if (isDebugMode) return "4-Tarjan";
            else return "4";
        }

        AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
            const int num_vertices = g.numVertices();
            if (num_vertices == 0) {
                return -1;
            }
            if (num_vertices == 1) {
                return 0;
            }

            std::vector<int> ids(num_vertices, -1);
            std::vector<int> low(num_vertices, -1);
            std::vector<bool> onStack(num_vertices, false);
            std::vector<int> st;
            st.reserve(num_vertices);

            std::vector<int> scc_map(num_vertices, -1);
            int scc_count = 0;
            int id_counter = 0;

            /*
            std::function<void(int)> tarjan_dfs =
                [&](int at) {
                st.push_back(at);
                onStack[at] = true;
                ids[at] = low[at] = id_counter++;

                for (int to : g.outneighbors(at)) {
                    if (ids[to] == -1) {
                        tarjan_dfs(to);
                        low[at] = std::min(low[at], low[to]);
                    } else if (onStack[to]) {
                        low[at] = std::min(low[at], ids[to]);
                    }
                }

                if (ids[at] == low[at]) {
                    while (true) {
                        int node = st.back();
                        st.pop_back();
                        onStack[node] = false;
                        scc_map[node] = scc_count;
                        if (node == at) break;
                    }
                    scc_count++;
                }
            };

            for (int i = 0; i < num_vertices; ++i) {
                if (ids[i] == -1) {
                    tarjan_dfs(i);
                }
            }
             */

            for (int i = 0; i < num_vertices; ++i) {
                if (ids[i] == -1) {
                    auto initial_neighbors = g.outneighbors(i);
                    using neighbor_iterator = decltype(initial_neighbors.begin());
                    std::vector<std::tuple<int, neighbor_iterator, neighbor_iterator>> dfs_stack;

                    dfs_stack.emplace_back(i, initial_neighbors.begin(), initial_neighbors.end());
                    st.push_back(i);
                    onStack[i] = true;
                    ids[i] = low[i] = id_counter++;

                    while(!dfs_stack.empty()) {
                        auto& [at, it, end_it] = dfs_stack.back();

                        bool pushed_new = false;
                        while(it != end_it) {
                            int to = *it;
                            ++it;

                            if (ids[to] == -1) {
                                st.push_back(to);
                                onStack[to] = true;
                                ids[to] = low[to] = id_counter++;
                                auto to_neighbors = g.outneighbors(to);
                                dfs_stack.emplace_back(to, to_neighbors.begin(), to_neighbors.end());
                                pushed_new = true;
                                break;
                            } else if (onStack[to]) {
                                low[at] = std::min(low[at], ids[to]);
                            }
                        }

                        if (!pushed_new) {
                            if (ids[at] == low[at]) {
                                while (true) {
                                    int node = st.back();
                                    st.pop_back();
                                    onStack[node] = false;
                                    scc_map[node] = scc_count;
                                    if (node == at) break;
                                }
                                scc_count++;
                            }

                            dfs_stack.pop_back();
                            if(!dfs_stack.empty()) {
                                auto& [parent, parent_it, parent_end_it] = dfs_stack.back();
                                low[parent] = std::min(low[parent], low[at]);
                            }
                        }
                    }
                }
            }

            std::vector<int> scc_in_degree(scc_count, 0);
            for (int u = 0; u < num_vertices; ++u) {
                for (int v : g.outneighbors(u)) {
                    if (scc_map[u] != scc_map[v]) {
                        scc_in_degree[scc_map[v]]++;
                    }
                }
            }

            int source_scc_id = -1;
            int source_scc_count = 0;
            for (int i = 0; i < scc_count; ++i) {
                if (scc_in_degree[i] == 0) {
                    source_scc_count++;
                    source_scc_id = i;
                }
            }

            if (source_scc_count != 1) {
                return -1;
            }

            int candidate_vertex = -1;
            for (int i = 0; i < num_vertices; ++i) {
                if (scc_map[i] == source_scc_id) {
                    candidate_vertex = i;
                    break;
                }
            }

            if (candidate_vertex == -1) {
                 return -1;
            }

            std::vector<bool> visited(num_vertices, false);
            std::vector<int> reach_count_vec;
            reach_count_vec.reserve(num_vertices);
            DFS_util(g, candidate_vertex, visited, reach_count_vec);

            if (reach_count_vec.size() != num_vertices) {
                return -1;
            }

            int min_vertex_in_source_scc = num_vertices;
            for (int i = 0; i < num_vertices; ++i) {
                if (scc_map[i] == source_scc_id) {
                    min_vertex_in_source_scc = std::min(min_vertex_in_source_scc, i);
                }
            }
            return min_vertex_in_source_scc;
        }
    };

    class PathBasedUniversalSourceFinderStrategy : public AlgorithmInterface {
        //based on: https://en.wikipedia.org/wiki/Path-based_strong_component_algorithm
    public:
        using solves_problem = Problem::FirstUniversalSource;
        using properties = AlgorithmProperties::SparseGraphPreferred;
        using preferred_graph_properties = GraphProperties::CacheLocal;
        using algorithm_interface = AlgorithmInterface;
        const char* getName() const override {
            if (isDebugMode) return "4-PathBased";
            else return "4";
        }

        AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
            const int num_vertices = g.numVertices();
            if (num_vertices == 0) return -1;
            if (num_vertices == 1) return 0;

            std::vector<int> preorder(num_vertices, 0);
            int preorder_counter = 1; // Start at 1, 0 means unvisited
            std::vector<int> S; S.reserve(num_vertices);
            std::vector<int> P; P.reserve(num_vertices);
            std::vector<int> scc_map(num_vertices, -1);
            int scc_count = 0;

            /*
            std::function<void(int)> path_based_dfs =
                [&](int v) {
                preorder[v] = preorder_counter++;
                S.push_back(v);
                P.push_back(v);
                for (int w : g.outneighbors(v)) {
                    if (preorder[w] == 0) {
                        path_based_dfs(w);
                    } else if (scc_map[w] == -1) {
                        while (!P.empty() && preorder[P.back()] > preorder[w]) {
                            P.pop_back();
                        }
                    }
                }

                if (!P.empty() && P.back() == v) {
                    P.pop_back();
                    while (true) {
                        int node = S.back();
                        S.pop_back();
                        scc_map[node] = scc_count;
                        if (node == v) break;
                    }
                    scc_count++;
                }
            };

            for (int i = 0; i < num_vertices; ++i) {
                if (preorder[i] == 0) {
                    path_based_dfs(i);
                }
            }
             */

            for (int i = 0; i < num_vertices; ++i) {
                if (preorder[i] == 0) {
                    std::vector<int> stack;

                    stack.emplace_back(i);
                    preorder[i] = preorder_counter++;
                    S.push_back(i);
                    P.push_back(i);

                    while(!stack.empty()) {
                        auto& v = stack.back();
                        auto it = g.outneighbors(v).begin();
                        auto end_it = g.outneighbors(v).end();


                        bool pushed_new = false;
                        while(it != end_it) {
                            int w = *it;
                            ++it;

                            if (preorder[w] == 0) {
                                auto w_neighbors = g.outneighbors(w);
                                stack.emplace_back(w);
                                preorder[w] = preorder_counter++;
                                S.push_back(w);
                                P.push_back(w);
                                pushed_new = true;
                                break;
                            } else if (scc_map[w] == -1) {
                                while (!P.empty() && preorder[P.back()] > preorder[w]) {
                                    P.pop_back();
                                }
                            }
                        }

                        if (!pushed_new) {
                            if (!P.empty() && P.back() == v) {
                                P.pop_back();
                                while (true) {
                                    int node = S.back();
                                    S.pop_back();
                                    scc_map[node] = scc_count;
                                    if (node == v) break;
                                }
                                scc_count++;
                            }
                            stack.pop_back();
                        }
                    }
                }
            }

            std::vector<int> scc_in_degree(scc_count, 0);
            for (int u = 0; u < num_vertices; ++u) {
                for (int v : g.outneighbors(u)) {
                    if (scc_map[u] != scc_map[v]) {
                        scc_in_degree[scc_map[v]]++;
                    }
                }
            }

            int source_scc_id = -1;
            int source_scc_count = 0;
            for (int i = 0; i < scc_count; ++i) {
                if (scc_in_degree[i] == 0) {
                    source_scc_count++;
                    source_scc_id = i;
                }
            }

            if (source_scc_count != 1) {
                return -1;
            }

            int candidate_vertex = -1;
            for (int i = 0; i < num_vertices; ++i) {
                if (scc_map[i] == source_scc_id) {
                    candidate_vertex = i;
                    break;
                }
            }

            if (candidate_vertex == -1) {
                return -1;
            }

            std::vector<bool> visited(num_vertices, false);
            std::vector<int> reach_count_vec;
            reach_count_vec.reserve(num_vertices);
            DFS_util(g, candidate_vertex, visited, reach_count_vec);

            if (reach_count_vec.size() != num_vertices) {
                return -1;
            }

            int min_vertex_in_source_scc = num_vertices;
            for (int i = 0; i < num_vertices; ++i) {
                if (scc_map[i] == source_scc_id) {
                    min_vertex_in_source_scc = std::min(min_vertex_in_source_scc, i);
                }
            }

            return (min_vertex_in_source_scc == num_vertices) ? -1 : min_vertex_in_source_scc;
        }
    };
};
