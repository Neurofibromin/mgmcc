# MGMCC

mediocre graph manager code compendium

## Graph library in C++23

This project implements patterned programming heavily.

## how to build

```shell
cmake -B build -S .
cmake --build build
```

## with gcc only

```shell
g++ -std=c++23 -fmodules-ts -o 3week src/main.cpp src/impl/GraphAMatrix.ixx src/impl/GraphFList.ixx src/impl/GraphNList.ixx 
src/impl/Profiler.ixx src/impl/SpanView.ixx src/algorithms/Generator.ixx src/algorithms/GraphAlgo.ixx src/core/AlgorithmDecorator.ixx
src/core/AlgorithmResult.ixx src/core/GraphConcepts.ixx src/core/Properties.ixx src/core/GraphPropertySelector.ixx
src/core/ImplementedGraph.ixx src/core/StrategySelector.ixx src/factories/DecoratorFactory.ixx src/factories/GraphFactory.ixx
src/factories/GraphProcessorAlgorithmStrategyFactory.ixx src/factories/StrategyProvider.ixx src/interfaces/IAlgorithm.ixx
src/interfaces/IGraph.ixx
```

## module wrapping

Currently, the project has lots of modules. Unite these under the mgmcc module (as module partitions). 
TODO: Put everything into the mgmcc namespace.

## optimizations
- [x] branchless code
- [x] change memory allocator
  - LD_PRELOAD=/usr/lib64/libjemalloc.so.2 cmake-build-release/mgmcc --profiling > jemalloc_profiling_results_3.txt
  - LD_PRELOAD=/usr/lib64/libtcmalloc.so.4 cmake-build-release/mgmcc --profiling > tcmalloc.txt
- [x] fix segv in GraphAMatrix
- [x] profiling with external tool
- [x] cacheline
- [x] cache invalidation
- [ ] add description for toolbox
- [ ] parallelisation
- [ ] parallelisation & gpu support with openMP

## profiling:

```shell
perf stat cmake-build-release/mgmcc --profiling
perf stat -e cache-references,cache-misses,L1-dcache-load-misses,LLC-load-misses,dTLB-load-misses,major-faults cmake-build-release/mgmcc --profiling
perf stat -e cycles,instructions,stalled-cycles-frontend,stalled-cycles-backend,bus-cycles cmake-build-release/mgmcc --profiling
perf stat -e cache-references,cache-misses,L1-dcache-load-misses,LLC-load-misses,dTLB-load-misses,major-faults,cycles,instructions,stalled-cycles-frontend,stalled-cycles-backend,bus-cycles cmake-build-release/mgmcc --profiling
```
Performance counter stats for 'cmake-build-release/mgmcc --profiling':
```terminaloutput
         77.833,00 msec task-clock:u                     #    1,022 CPUs utilized             
                 0      context-switches:u               #    0,000 /sec                      
                 0      cpu-migrations:u                 #    0,000 /sec                      
           104.492      page-faults:u                    #    1,343 K/sec                     
 1.795.995.638.724      instructions:u                   #    4,33  insn per cycle            
                                                         #    0,01  stalled cycles per insn   
   414.864.363.540      cycles:u                         #    5,330 GHz                       
    22.840.380.486      stalled-cycles-frontend:u        #    5,51% frontend cycles idle      
   345.330.223.742      branches:u                       #    4,437 G/sec                     
       913.100.170      branch-misses:u                  #    0,26% of all branches
```

Performance counter stats for 'cmake-build-release/mgmcc --profiling':

```terminaloutput
    85.734.768.739      cache-references:u                                                      (71,46%)
     6.262.625.508      cache-misses:u                   #    7,30% of all cache refs           (71,34%)
    45.207.702.695      L1-dcache-load-misses:u                                                 (71,32%)
   <not supported>      LLC-load-misses:u                                                     
        12.687.721      dTLB-load-misses:u                                                      (71,41%)
                 0      major-faults:u                                                        
   423.823.388.371      cycles:u                                                                (71,46%)
 1.796.696.983.823      instructions:u                   #    4,24  insn per cycle            
                                                         #    0,01  stalled cycles per insn     (71,50%)
    22.842.693.489      stalled-cycles-frontend:u        #    5,39% frontend cycles idle        (71,51%)
   <not supported>      stalled-cycles-backend:u                                              
   <not supported>      bus-cycles:u                                                          
```

### more details with record

```shell
perf record -g --call-graph dwarf cmake-build-release/mgmcc --profiling 2 #generates perf.data
perf report #use e to expand call graph
```

### FlameGraph

```shell
git clone https://github.com/brendangregg/FlameGraph.git
perf script | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl > perf.svg
```



## won't implement

- [ ] graph edge lists could be ordered for faster check_if_edge_in_graph, but such functionality is not needed (use binary search)
  - order after every insert? or keep a std::vector<bool> isordered to note which is no longer ordered and only order the unordered ones
  - parallel ordering: every edge list is independent, only woth it for large lists and many vertices
- [ ] graph edge lists could be std::map for the same reason, but then no parallel edges?

### SpanView:

SpanView should have std::span<const int> and std::generator<const int> backends, so a SpanView can be returned by all implementations (GraphFList, GraphNList, GraphAMatrix).
Aim: zero runtime cost
Note: std::ranges::any_view is not available in c++23 yet.
