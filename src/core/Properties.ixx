export module Properties;

export namespace GraphProperties {
    struct CacheLocal {};

    struct EasilyMutable {};

    struct NoPreference {};
}

export namespace AlgorithmProperties {
    struct DenseGraphPreferred {};
    struct SparseGraphPreferred {};
    struct NoPreference {};
}

export namespace Problem {
    struct SourceVertexCount {}; //problem 1
    struct DiameterMeasure {}; //problem 2
    struct FeedbackArcSet {}; //problem 3
    struct FirstUniversalSource {}; //problem 4
}