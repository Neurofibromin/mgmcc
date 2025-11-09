export module IAlgorithm;

import AlgorithmResult;
class ImplementedGraph;

export template <typename GraphTypeImplementationGeneralizer = ImplementedGraph>
class IAlgorithm {
public:
    virtual ~IAlgorithm() = default;
    virtual AlgoResultVariant execute(const GraphTypeImplementationGeneralizer& g) const = 0;
    virtual const char* getName() const = 0;

    using implementation_generalizer_type = GraphTypeImplementationGeneralizer;
};