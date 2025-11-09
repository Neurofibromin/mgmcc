module;

# include <memory>
# include <chrono>
# include <iostream>
# include <iomanip>
# include <functional>

export module AlgorithmDecorator;

import IAlgorithm;
import AlgorithmResult;
import GraphConcepts;
import ImplementedGraph;
import GraphPropertySelector;
import Properties;

export template <IsGraph GraphTypeImplementationGeneralizer = ImplementedGraph,
                 typename AlgorithmInterface = IAlgorithm<GraphTypeImplementationGeneralizer>>
requires std::is_same_v<GraphTypeImplementationGeneralizer, typename AlgorithmInterface::implementation_generalizer_type>
class TimingDecorator : public AlgorithmInterface {
private:
    std::unique_ptr<AlgorithmInterface> wrapped_algo;

public:
    explicit TimingDecorator(std::unique_ptr<AlgorithmInterface> algo)
        : wrapped_algo(std::move(algo)) {}

    const char* getName() const override {
        return wrapped_algo->getName();
    }

    AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
        std::cout << "" << getName() << ":" << std::endl;
        const auto start = std::chrono::steady_clock::now();

        auto result = wrapped_algo->execute(g);

        const auto finish = std::chrono::steady_clock::now();
        const std::chrono::duration<double> elapsed = finish - start;

        std::cout << "time: " << std::fixed << std::setprecision(9) << elapsed.count() << "s\n";
        return result;
    }
};

export template <IsGraph GraphTypeImplementationGeneralizer = ImplementedGraph,
                 typename AlgorithmInterface = IAlgorithm<GraphTypeImplementationGeneralizer>>
requires std::is_same_v<GraphTypeImplementationGeneralizer, typename AlgorithmInterface::implementation_generalizer_type>
class AutoImplementationChangerGraphStrategyExecutor : public AlgorithmInterface {
private:
    std::unique_ptr<AlgorithmInterface> wrapped_algo;
    std::function<void(const GraphTypeImplementationGeneralizer&)> conversion_fn;

public:
    // to deduce the concrete algorithm type
    template <typename ConcreteAlgorithm>
    explicit AutoImplementationChangerGraphStrategyExecutor(std::unique_ptr<ConcreteAlgorithm> algo)
        : wrapped_algo(std::move(algo))
    {
        // type erasure
        this->conversion_fn = [](const GraphTypeImplementationGeneralizer& g) {
            using PreferredProperty = typename ConcreteAlgorithm::preferred_graph_properties;
            if constexpr (std::is_same_v<PreferredProperty, GraphProperties::NoPreference>) {
                return;
            }
            constexpr bool is_supported =
                (std::is_same_v<PreferredProperty, GraphProperties::CacheLocal> && GraphTypeImplementationGeneralizer::is_cache_local::value) ||
                (std::is_same_v<PreferredProperty, GraphProperties::EasilyMutable> && GraphTypeImplementationGeneralizer::is_easily_mutable::value);

            if constexpr (is_supported) {
                using TargetGraphType = typename GraphImplementationPropertyProviderSelector<
                    PreferredProperty,
                    typename GraphTypeImplementationGeneralizer::graph_variant
                >::type;
                if constexpr (!std::is_same_v<TargetGraphType, void>) {
                    g.template convertTo<TargetGraphType>();
                }
            }
        };
    }

    const char* getName() const override {
        return wrapped_algo->getName();
    }

    AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const override {
        conversion_fn(g);
        return wrapped_algo->execute(g);
    }
};