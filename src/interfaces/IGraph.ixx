module;

# include <span>
# include <vector>

export module IGraph;

export class IGraph {
public:
    IGraph();
    virtual ~IGraph();

    IGraph(const IGraph &);
    IGraph(IGraph &&);
    IGraph &operator=(const IGraph &);
    IGraph &operator=(IGraph &&);

    virtual int numVertices() const = 0;
    virtual int numEdges() const = 0;
    virtual void addEdge(int u, int v) = 0;
    virtual void removeEdge(int u, int v) = 0;
    virtual std::span<const int> outneighbors(int u) const = 0;
    virtual std::span<const int> inneighbors(int u) const = 0;
    virtual int out_degree(int u) const = 0;
    virtual int in_degree(int u) const = 0;
    // virtual IGraph& getTranspose() const = 0;
};

//linker problems
inline IGraph::IGraph() = default;
inline IGraph::~IGraph() = default;
inline IGraph::IGraph(const IGraph &) = default;
inline IGraph::IGraph(IGraph &&) = default;
inline IGraph &IGraph::operator=(const IGraph &) = default;
inline IGraph &IGraph::operator=(IGraph &&) = default;