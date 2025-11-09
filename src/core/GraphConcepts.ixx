module;

#include  <span>
#include  <concepts>
#include  <variant>

export module GraphConcepts;

export template <typename G>
concept IsGraph = requires(G g, const G& cg, int u, int v) {
    { cg.numVertices() } -> std::same_as<int>;
    { cg.numEdges() } -> std::same_as<int>;
    { g.addEdge(u, v) } -> std::same_as<void>;
    { g.removeEdge(u, v) } -> std::same_as<void>;
    { cg.outneighbors(u) } -> std::same_as<std::span<const int>>;
    { cg.inneighbors(u) } -> std::same_as<std::span<const int>>;
    { cg.out_degree(u) } -> std::same_as<int>;
    { cg.in_degree(u) } -> std::same_as<int>;
    { cg.getTranspose() } -> std::same_as<G>;
};

template<typename T, typename Variant>
struct is_in_variant;

template<typename T, typename... Types>
struct is_in_variant<T, std::variant<Types...>> : std::disjunction<std::is_same<T, Types>...> {};

export template<typename T, typename Variant>
concept IsVariantMember = is_in_variant<T, Variant>::value;