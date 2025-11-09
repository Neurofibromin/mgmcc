module;

#include  <memory>

export module DecoratorFactory;

import IAlgorithm;
import AlgorithmDecorator;
import ImplementedGraph;

export template <
    typename ConcreteAlgorithm,
    bool TimeConversion = true, // Default to timing the conversion as well
    typename AlgorithmInterface = ConcreteAlgorithm::algorithm_interface
>
requires std::is_same_v<typename ConcreteAlgorithm::algorithm_interface, AlgorithmInterface>
std::unique_ptr<AlgorithmInterface> make_decorated_algorithm() {

    auto base_algo = std::make_unique<ConcreteAlgorithm>();

    if constexpr (TimeConversion) {
        // Time(VariantSelector(Algo))
        auto variant_selector = std::make_unique<AutoImplementationChangerGraphStrategyExecutor<>>(std::move(base_algo));
        return std::make_unique<TimingDecorator<>>(std::move(variant_selector));
    } else {
        // VariantSelector(Time(Algo))
        auto timer = std::make_unique<TimingDecorator<>>(std::move(base_algo));
        return std::make_unique<AutoImplementationChangerGraphStrategyExecutor<>>(std::move(timer));
    }
}