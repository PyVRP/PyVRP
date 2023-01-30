cmake_minimum_required(VERSION 3.15)

set(diversitySources
        diversity/brokenPairsDistance.cpp)

set(geneticSources
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
set(xorShift128Sources XorShift128.cpp)

# Internal libraries
add_library(xorShift128 ${xorShift128Sources})

add_library(penaltyManager ${penaltySources})

add_library(problemData ${problemDataSources})

add_library(individual ${individualSources})
target_link_libraries(individual problemData penaltyManager xorShift128)

add_library(diversityMeasures ${diversitySources})
target_link_libraries(diversityMeasures individual problemData)

add_library(tws ${timeWindowSegmentSources})

add_library(population ${populationSources})
target_link_libraries(population individual problemData xorShift128)


# TODO ok up to here

add_library(genetic ${geneticSources})
target_link_libraries(genetic xorShift128)

add_library(ls ${localSearchSources})
target_link_libraries(ls xorShift128)

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
        tws
        xorShift128)

add_subdirectory(python)
