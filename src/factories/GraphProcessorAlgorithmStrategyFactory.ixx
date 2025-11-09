module;

#include <memory>

export module GraphProcessorAlgorithmStrategyFactory;

import GraphAlgo;
import IAlgorithm;
import GraphConcepts;

class ImplementedGraph;

export
template <
    IsGraph GraphTypeImplementationGeneralizer = ImplementedGraph,
    typename AlgorithmInterface = IAlgorithm<GraphTypeImplementationGeneralizer>,
    bool isDebugMode = false
>
class GraphProcessorAlgorithmStrategyFactory {
public:
    using ProcessorType = GraphProcessor<GraphTypeImplementationGeneralizer, AlgorithmInterface, isDebugMode>;

    std::unique_ptr<AlgorithmInterface> createSourceVertexStrategy() const {
        return std::make_unique<typename ProcessorType::SourceVertexStrategy>();
    }

    std::unique_ptr<AlgorithmInterface> createSequentialDiameterStrategy() const {
        return std::make_unique<typename ProcessorType::SequentialDiameterStrategy>();
    }

    std::unique_ptr<AlgorithmInterface> createAsyncDiameterStrategy() const {
        return std::make_unique<typename ProcessorType::AsyncDiameterStrategy>();
    }

    std::unique_ptr<AlgorithmInterface> createParallelDiameterStrategy() const {
        return std::make_unique<typename ProcessorType::ParallelDiameterStrategy>();
    }

    std::unique_ptr<AlgorithmInterface> createFeedbackArcSetRemoveCyclesStrategy() const {
        return std::make_unique<typename ProcessorType::FeedbackArcSetRemoveCyclesStrategy>();
    }

    std::unique_ptr<AlgorithmInterface> createFeedbackArcSetInsertEdgesStrategy() const {
        return std::make_unique<typename ProcessorType::FeedbackArcSetInsertEdgesStrategy>();
    }

    std::unique_ptr<AlgorithmInterface> createFeedbackArcSetDfsStrategy() const {
        return std::make_unique<typename ProcessorType::FeedbackArcSetDfsStrategy>();
    }

    std::unique_ptr<AlgorithmInterface> createSequentialUniversalSourceFinderStrategy() const {
        return std::make_unique<typename ProcessorType::SequentialUniversalSourceFinderStrategy>();
    }

    std::unique_ptr<AlgorithmInterface> createParallelUniversalSourceFinderStrategy() const {
        return std::make_unique<typename ProcessorType::ParallelUniversalSourceFinderStrategy>();
    }

    std::unique_ptr<AlgorithmInterface> createKosarajuUniversalSourceFinderStrategy() const {
        return std::make_unique<typename ProcessorType::KosarajuUniversalSourceFinderStrategy>();
    }

    std::unique_ptr<AlgorithmInterface> createTarjanUniversalSourceFinderStrategy() const {
        return std::make_unique<typename ProcessorType::TarjanUniversalSourceFinderStrategy>();
    }

    std::unique_ptr<AlgorithmInterface> createPathBasedUniversalSourceFinderStrategy() const {
        return std::make_unique<typename ProcessorType::PathBasedUniversalSourceFinderStrategy>();
    }
};