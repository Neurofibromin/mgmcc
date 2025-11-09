module;

# include <memory>

export module StrategyProvider;

import StrategySelector;
import GraphAlgo;
import ImplementedGraph;
import IAlgorithm;
import GraphConcepts;

export template<IsGraph GraphTypeImplementationGeneralizer = ImplementedGraph,
                typename AlgorithmInterface = IAlgorithm<GraphTypeImplementationGeneralizer>,
                bool isDebugMode = false>
requires std::is_same_v<GraphTypeImplementationGeneralizer, typename AlgorithmInterface::implementation_generalizer_type>
struct StrategyProvider {
public:
    using Processor = GraphProcessor<GraphTypeImplementationGeneralizer, AlgorithmInterface, isDebugMode>;
    using type = StrategySelector<GraphTypeImplementationGeneralizer, AlgorithmInterface,
        typename Processor::SourceVertexStrategy,
        typename Processor::SequentialDiameterStrategy,
        typename Processor::AsyncDiameterStrategy,
        typename Processor::ParallelDiameterStrategy,
        typename Processor::FeedbackArcSetRemoveCyclesStrategy,
        typename Processor::FeedbackArcSetInsertEdgesStrategy,
        typename Processor::FeedbackArcSetDfsStrategy,
        typename Processor::SequentialUniversalSourceFinderStrategy,
        typename Processor::ParallelUniversalSourceFinderStrategy,
        typename Processor::KosarajuUniversalSourceFinderStrategy,
        typename Processor::TarjanUniversalSourceFinderStrategy,
        typename Processor::PathBasedUniversalSourceFinderStrategy
    >;
};