module;

#include <span>
#include <variant>
#include <utility>
#include <iostream>
#include <map>
#include <memory>
#include <typeindex>
#include <concepts>
#include <type_traits>

import GraphConcepts;
import IGraph;
import GraphNList;
import GraphFList;
import GraphAMatrix;

//intends to provide something like pimpl, so the algos can work on ImplementedGraph-s,
//and the factory can take the desired graph implementation type as a template param and return an ImplementedGraph wrapping that type

export module ImplementedGraph;

using GraphVariant = std::variant<GraphNList, GraphFList, GraphAMatrix>;

namespace traitdetector {
    template <typename G>
    concept HasCacheLocalTrait = requires { { G::is_cache_local::value } -> std::same_as<const bool&>; };
    template<typename G>
    struct get_is_cache_local : std::bool_constant<HasCacheLocalTrait<G> ? G::is_cache_local::value : false> {};

    template <typename G>
    concept HasEasilyMutableTrait = requires { { G::is_easily_mutable::value } -> std::same_as<const bool&>; };
    template<typename G>
    struct get_is_easily_mutable : std::bool_constant<HasEasilyMutableTrait<G> ? G::is_easily_mutable::value : false> {};
    template <typename Variant, template<typename> typename Predicate, size_t I = 0>
    consteval bool any_type_satisfies() {
        if constexpr (I >= std::variant_size_v<Variant>) {
            return false;
        } else {
            using T = std::variant_alternative_t<I, Variant>;
            if constexpr (Predicate<T>::value) {
                return true;
            } else {
                return any_type_satisfies<Variant, Predicate, I + 1>();
            }
        }
    }
}

export class ImplementedGraph : public IGraph {
private:
    mutable GraphVariant graph_impl;
    mutable std::map<std::type_index, GraphVariant> stashed_graph_impls;
public:
    using graph_variant = ::GraphVariant;
    using graph_interface = IGraph;

    using is_cache_local = std::bool_constant<traitdetector::any_type_satisfies<GraphVariant, traitdetector::get_is_cache_local>()>;
    using is_easily_mutable = std::bool_constant<traitdetector::any_type_satisfies<GraphVariant, traitdetector::get_is_easily_mutable>()>;

    template <IsGraph G>
    explicit ImplementedGraph(G&& graph) requires std::constructible_from<GraphVariant, G&&>
    && std::is_base_of_v<IGraph, G> && IsVariantMember<G, GraphVariant>
        : IGraph(), graph_impl(std::forward<G>(graph)), stashed_graph_impls() {}

    ImplementedGraph(const ImplementedGraph &) = default;
    ImplementedGraph(ImplementedGraph &&) = default;
    ImplementedGraph &operator=(const ImplementedGraph &) = default;
    ImplementedGraph &operator=(ImplementedGraph &&) = default;

    //converts graph in place
    template <IsGraph NewGraphImplementationType>
    void convertTo() const
    requires std::constructible_from<GraphVariant, NewGraphImplementationType&&>
    && std::is_base_of_v<IGraph, NewGraphImplementationType> && IsVariantMember<NewGraphImplementationType, GraphVariant>
    {
        // if NewGraphImplementationType == current implementation: return early
        if (std::holds_alternative<NewGraphImplementationType>(graph_impl)) {
            return;
        }

        // stash the current implementation away
        std::visit([this](const auto& concrete_graph){
            stashed_graph_impls.insert_or_assign(std::type_index(typeid(concrete_graph)), graph_impl);
        }, graph_impl);

        // lookup-before-construct logic to pull from stashed_graph_impls cache if possible
        if (auto it = stashed_graph_impls.find(std::type_index(typeid(NewGraphImplementationType))); it != stashed_graph_impls.end()) {
            graph_impl = it->second;
            return;
        }
        //lookup failed
        graph_impl = std::visit(
            [](const auto& concrete_graph) {
                return GraphVariant(NewGraphImplementationType(concrete_graph));
            },
            graph_impl
        );
    }

    const GraphVariant& getVariant() const {
        return graph_impl;
    }

    int numVertices() const override {
        return std::visit([](const auto& g) { return g.numVertices(); }, graph_impl);
    }

    int numEdges() const override {
        return std::visit([](const auto& g) { return g.numEdges(); }, graph_impl);
    }

    void addEdge(int u, int v) override {
        stashed_graph_impls.clear();
        std::visit([=](auto& g) { g.addEdge(u, v); }, graph_impl);
    }

    void removeEdge(int u, int v) override {
        stashed_graph_impls.clear();
        std::visit([=](auto& g) { g.removeEdge(u, v); }, graph_impl);
    }

    std::span<const int> outneighbors(int u) const override {
        return std::visit([=](const auto& g) { return g.outneighbors(u); }, graph_impl);
    }

    std::span<const int> inneighbors(int u) const override {
        return std::visit([=](const auto& g) { return g.inneighbors(u); }, graph_impl);
    }

    int out_degree(int u) const override {
        return std::visit([=](const auto& g) { return g.out_degree(u); }, graph_impl);
    }

    int in_degree(int u) const override {
        return std::visit([=](const auto& g) { return g.in_degree(u); }, graph_impl);
    }

    ImplementedGraph getTranspose() const {
        return std::visit(
            [](const auto& g) {
                return ImplementedGraph(g.getTranspose());
            }, graph_impl);
    }
};

export std::ostream& operator<<(std::ostream& ostream, const ImplementedGraph& g) {
    for (int i = 0; i < g.numVertices(); ++i) {
        ostream << i << " -> [ ";
        for (int neighbor : g.outneighbors(i)) {
            ostream << neighbor << " ";
        }
        ostream << "]" << std::endl;
    }
    return ostream;
}