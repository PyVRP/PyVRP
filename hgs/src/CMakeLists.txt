cmake_minimum_required(VERSION 3.2)

set(geneticSources
        GeneticAlgorithm.cpp
        crossover/crossover.cpp
        crossover/selectiveRouteExchange.cpp)

set(individualSources
        Individual.cpp)

set(localSearchSources
        LocalSearch.cpp
        operators/Exchange.cpp
        operators/MoveTwoClientsReversed.cpp
        operators/RelocateStar.cpp
        operators/SwapStar.cpp
        operators/TwoOpt.cpp)

set(nodeSources
        Node.cpp)

set(paramsSources
        Params.cpp)

set(populationSources
        Population.cpp)

set(resultSources
        Result.cpp)

set(routeSources
        Route.cpp)

set(statisticsSources
        Statistics.cpp)

set(timeWindowSegmentSources
        TimeWindowSegment.cpp)

# (Internal) library
add_library(hgs
        ${geneticSources}
        ${individualSources}
        ${localSearchSources}
        ${nodeSources}
        ${paramsSources}
        ${populationSources}
        ${resultSources}
        ${routeSources}
        ${statisticsSources}
        ${timeWindowSegmentSources})

# Executable
add_executable(genvrp main.cpp)
target_link_libraries(genvrp PRIVATE hgs)

# Python bindings
pybind11_add_module(hgspy bindings.cpp)
target_link_libraries(hgspy PRIVATE hgs)
