cmake_minimum_required(VERSION 3.2)

set(geneticSources
        GeneticAlgorithm.cpp
        crossover/crossover.cpp
        crossover/selectiveRouteExchange.cpp)

set(localSearchSources
        LocalSearch.cpp
        operators/Exchange.cpp
        operators/MoveTwoClientsReversed.cpp
        operators/RelocateStar.cpp
        operators/SwapStar.cpp
        operators/TwoOpt.cpp)

set(individualSources Individual.cpp)
set(nodeSources Node.cpp)
set(paramsSources Params.cpp)
set(penaltySources PenaltyManager.cpp)
set(populationSources Population.cpp)
set(resultSources Result.cpp)
set(routeSources Route.cpp)
set(statisticsSources Statistics.cpp)
set(timeWindowSegmentSources TimeWindowSegment.cpp)

# Internal libraries
add_library(individual ${individualSources})
add_library(genetic ${geneticSources})
add_library(ls ${localSearchSources})
add_library(node ${nodeSources})
add_library(params ${paramsSources})
add_library(penaltyManager ${penaltySources})
add_library(population ${populationSources})
add_library(result ${resultSources})
add_library(route ${routeSources})
add_library(statistics ${statisticsSources})
add_library(tws ${timeWindowSegmentSources})

set(hgs_libs
        individual
        genetic
        ls
        node
        params
        penaltyManager
        population
        result
        route
        statistics
        tws)

# Executable
add_executable(hgs main.cpp)
target_link_libraries(hgs PRIVATE ${hgs_libs})

# Python bindings
pybind11_add_module(hgspy bindings.cpp)
target_link_libraries(hgspy PRIVATE ${hgs_libs})
