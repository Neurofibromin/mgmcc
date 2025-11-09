cmake_minimum_required(VERSION 4.0)
project(mgmcc)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

add_library(mgmcc_lib STATIC)

target_sources(mgmcc_lib PUBLIC FILE_SET CXX_MODULES FILES
               src/impl/GraphAMatrix.ixx
               src/impl/GraphFList.ixx
               src/impl/GraphNList.ixx
               src/impl/Profiler.ixx
               src/impl/SpanView.ixx
               src/algorithms/Generator.ixx
               src/algorithms/GraphAlgo.ixx
               src/core/AlgorithmDecorator.ixx
               src/core/AlgorithmResult.ixx
               src/core/GraphConcepts.ixx
               src/core/Properties.ixx
               src/core/GraphPropertySelector.ixx
               src/core/ImplementedGraph.ixx
               src/core/StrategySelector.ixx
               src/factories/DecoratorFactory.ixx
               src/factories/GraphFactory.ixx
               src/factories/GraphProcessorAlgorithmStrategyFactory.ixx
               src/factories/StrategyProvider.ixx
               src/interfaces/IAlgorithm.ixx
               src/interfaces/IGraph.ixx
)

add_executable(mgmcc src/main.cpp)
target_link_libraries(mgmcc PRIVATE mgmcc_lib)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(mgmcc PRIVATE DEBUG)
endif()

# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -fno-omit-frame-pointer") # for profiling

add_subdirectory(tests)