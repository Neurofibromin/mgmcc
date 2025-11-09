module;

# include <memory>

export module GraphFactory;

class ImplementedGraph;
class GraphNList;

export
template <typename GraphTypeImplementationGeneralizer = ImplementedGraph>
class GraphFactory {
public:
    template <typename GraphImplementationType = GraphNList>
    static std::unique_ptr<GraphTypeImplementationGeneralizer> createGraph(int num_vertices) {
        return std::make_unique<GraphTypeImplementationGeneralizer>(GraphImplementationType(num_vertices));
    }

    using implementation_generalizer_type = GraphTypeImplementationGeneralizer;
};