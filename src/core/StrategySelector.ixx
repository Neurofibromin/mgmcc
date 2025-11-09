module;

#include <type_traits>
#include <memory>
#include <iostream>

export module StrategySelector;

import IAlgorithm;
import ImplementedGraph;
import GraphConcepts;
import Properties;
import GraphAlgo;
import AlgorithmDecorator;
import AlgorithmResult;
import DecoratorFactory;

export template <IsGraph GraphTypeImplementationGeneralizer = ImplementedGraph,
                 typename AlgorithmInterface = IAlgorithm<GraphTypeImplementationGeneralizer>,
                 typename... Algorithms>
requires std::is_same_v<GraphTypeImplementationGeneralizer, typename AlgorithmInterface::implementation_generalizer_type>
class StrategySelector {
private:
    static bool is_dense(const GraphTypeImplementationGeneralizer& g) {
        if (g.numVertices() == 0) return false;
        int v = g.numVertices();
        int e = g.numEdges();
        // Heuristic
        return e > (v * v / 4);
    }

    template <typename Problem, typename DensityProperty, typename... Algos>
    struct find_perfect_match;

    template <typename Problem, typename DensityProperty>
    struct find_perfect_match<Problem, DensityProperty> {
        using type = void; // no match
    };

    template <typename Problem, typename DensityProperty, typename First, typename... Rest>
    struct find_perfect_match<Problem, DensityProperty, First, Rest...> {
        using type = std::conditional_t<
            std::is_same_v<typename First::solves_problem, Problem> &&
            std::is_same_v<typename First::properties, DensityProperty>,
            First,
            typename find_perfect_match<Problem, DensityProperty, Rest...>::type
        >;
    };

    template <typename Problem, typename... Algos>
    struct find_no_preference_match;

    template <typename Problem>
    struct find_no_preference_match<Problem> {
        using type = void;
    };

    template <typename Problem, typename First, typename... Rest>
    struct find_no_preference_match<Problem, First, Rest...> {
        using type = std::conditional_t<
            std::is_same_v<typename First::solves_problem, Problem> &&
            std::is_same_v<typename First::properties, AlgorithmProperties::NoPreference>,
            First,
            typename find_no_preference_match<Problem, Rest...>::type
        >;
    };

    template<typename Problem, typename Processor>
    struct fallback_selector;

    template<typename Processor>
    struct fallback_selector<Problem::SourceVertexCount, Processor> {
        using type = typename Processor::SourceVertexStrategy;
    };
    template<typename Processor>
    struct fallback_selector<Problem::DiameterMeasure, Processor> {
        using type = typename Processor::AsyncDiameterStrategy;
    };
    template<typename Processor>
    struct fallback_selector<Problem::FeedbackArcSet, Processor> {
        using type = typename Processor::FeedbackArcSetDfsStrategy;
    };
    template<typename Processor>
    struct fallback_selector<Problem::FirstUniversalSource, Processor> {
        using type = typename Processor::TarjanUniversalSourceFinderStrategy;
    };


public:
    template <typename Problem, bool isDebugMode = false>
    static AlgoResultVariant solve(const GraphTypeImplementationGeneralizer& g) {
        using Processor = GraphProcessor<GraphTypeImplementationGeneralizer, AlgorithmInterface, isDebugMode>;

        using dense_prop = AlgorithmProperties::DenseGraphPreferred;
        using sparse_prop = AlgorithmProperties::SparseGraphPreferred;

        using fallback_algo = typename fallback_selector<Problem, Processor>::type;

        if (is_dense(g)) {
            using perfect_match = typename find_perfect_match<Problem, dense_prop, Algorithms...>::type;
            using no_pref_match = typename find_no_preference_match<Problem, Algorithms...>::type;

            if constexpr (!std::is_same_v<perfect_match, void>) {
                 auto decorated_algo = make_decorated_algorithm<perfect_match>();
                 if (isDebugMode) std::cout << "[StrategySelector] Selected dense strategy: " << decorated_algo->getName() << std::endl;
                 return decorated_algo->execute(g);
            } else if constexpr (!std::is_same_v<no_pref_match, void>) {
                auto decorated_algo = make_decorated_algorithm<no_pref_match>();
                if (isDebugMode) std::cout << "[StrategySelector] Selected no-preference strategy for dense graph: " << decorated_algo->getName() << std::endl;
                return decorated_algo->execute(g);
            } else {
                auto decorated_algo = make_decorated_algorithm<fallback_algo>();
                if (isDebugMode) std::cout << "[StrategySelector] Selected fallback strategy for dense graph: " << decorated_algo->getName() << std::endl;
                return decorated_algo->execute(g);
            }
        } else { // Sparse
            using perfect_match = typename find_perfect_match<Problem, sparse_prop, Algorithms...>::type;
            using no_pref_match = typename find_no_preference_match<Problem, Algorithms...>::type;

            if constexpr (!std::is_same_v<perfect_match, void>) {
                auto decorated_algo = make_decorated_algorithm<perfect_match>();
                if (isDebugMode) std::cout << "[StrategySelector] Selected sparse strategy: " << decorated_algo->getName() << std::endl;
                return decorated_algo->execute(g);
            } else if constexpr (!std::is_same_v<no_pref_match, void>) {
                auto decorated_algo = make_decorated_algorithm<no_pref_match>();
                if (isDebugMode) std::cout << "[StrategySelector] Selected no-preference strategy for sparse graph: " << decorated_algo->getName() << std::endl;
                return decorated_algo->execute(g);
            } else {
                auto decorated_algo = make_decorated_algorithm<fallback_algo>();
                if (isDebugMode) std::cout << "[StrategySelector] Selected fallback strategy for sparse graph: " << decorated_algo->getName() << std::endl;
                return decorated_algo->execute(g);
            }
        }
    }
};