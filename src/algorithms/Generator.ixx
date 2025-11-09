module;

#include <vector>
#include <utility>
#include <random>

export module Generator;

export [[nodiscard]]
std::vector<std::pair<int, int>> generate_erdos_renyi_edges(int vertex_count, int edge_count) {
    if (vertex_count <= 0) {
        return {};
    }
    if (edge_count <= 0) {
        return {};
    }
    std::vector<std::pair<int, int>> edges;
    edges.reserve(edge_count);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> vertex_dist(0, vertex_count - 1);
    for (int i = 0; i < edge_count; ++i) {
        int u = vertex_dist(gen);
        int v = vertex_dist(gen);
        edges.emplace_back(u, v);
    }

    return edges;
}