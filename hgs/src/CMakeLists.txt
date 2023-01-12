cmake_minimum_required(VERSION 3.2)

set(diversitySources
        diversity/brokenPairsDistance.cpp)

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
set(problemDataSources ProblemData.cpp)
set(penaltySources PenaltyManager.cpp)
set(populationSources Population.cpp)
set(resultSources Result.cpp)
set(routeSources Route.cpp)
set(statisticsSources Statistics.cpp)
set(timeWindowSegmentSources TimeWindowSegment.cpp)

# Internal libraries
add_library(penaltyManager ${penaltySources})

add_library(problemData ${problemDataSources})
target_link_libraries(problemData)

add_library(individual ${individualSources})
target_link_libraries(individual problemData penaltyManager)

add_library(diversityMeasures ${diversitySources})
target_link_libraries(diversityMeasures individual problemData)

add_library(tws ${timeWindowSegmentSources})

add_library(population ${populationSources})
target_link_libraries(population individual problemData)

# TODO ok up to here

add_library(genetic ${geneticSources})
add_library(ls ${localSearchSources})
add_library(node ${nodeSources})
add_library(result ${resultSources})
add_library(route ${routeSources})
add_library(statistics ${statisticsSources})

set(hgs_libs
        diversityMeasures
        individual
        genetic
        ls
        node
        problemData
        penaltyManager
        population
        result
        route
        statistics
        tws)

# Profiling executable
add_executable(profiling main.cpp)
target_link_libraries(profiling PRIVATE ${hgs_libs})

# Python library
pybind11_add_module(hgspy bindings.cpp)
target_link_libraries(hgspy PRIVATE ${hgs_libs})
