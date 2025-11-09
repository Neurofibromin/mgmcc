module;

#include <variant>
#include <type_traits>

export module GraphPropertySelector;

import Properties;

namespace detail {
    template <typename G, typename P>
    struct graph_has_property : std::false_type {};
    template <typename G>
    struct graph_has_property<G, GraphProperties::CacheLocal> {
        static constexpr bool value = G::is_cache_local::value;
    };
    template <typename G>
    struct graph_has_property<G, GraphProperties::EasilyMutable> {
        static constexpr bool value = G::is_easily_mutable::value;
    };

    template <typename P, typename Variant, std::size_t I = 0>
    struct find_first_graph_with_property {
        using type = void;
    };
    template <typename P, typename Variant, std::size_t I>
    requires (I < std::variant_size_v<Variant>)
    struct find_first_graph_with_property<P, Variant, I> {
        using current_graph_t = std::variant_alternative_t<I, Variant>;
        using type = std::conditional_t<
            graph_has_property<current_graph_t, P>::value,
            current_graph_t,
            typename find_first_graph_with_property<P, Variant, I + 1>::type
        >;
    };
}

export template <typename Property, typename GraphVariant>
struct GraphImplementationPropertyProviderSelector {
    using type = typename detail::find_first_graph_with_property<Property, GraphVariant>::type;
};