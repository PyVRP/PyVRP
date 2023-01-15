cmake_minimum_required(VERSION 3.2)

set(bindingSources
        bindings.cpp
        bind_crossover.cpp
        bind_diversity.cpp
        bind_Exchange.cpp
        bind_GeneticAlgorithm.cpp
        bind_GeneticAlgorithmParams.cpp
        bind_Individual.cpp
        bind_LocalSearch.cpp
        bind_LocalSearchOperator.cpp
        bind_LocalSearchParams.cpp
        bind_MaxIterations.cpp
        bind_MaxRuntime.cpp
        bind_MoveTwoClientsReversed.cpp
        bind_NoImprovement.cpp
        bind_PenaltyManager.cpp
        bind_PenaltyParams.cpp
        bind_Population.cpp
        bind_PopulationParams.cpp
        bind_ProblemData.cpp
        bind_RelocateStar.cpp
        bind_Result.cpp
        bind_Statistics.cpp
        bind_StoppingCriterion.cpp
        bind_SwapStar.cpp
        bind_TimedNoImprovement.cpp
        bind_TwoOpt.cpp
        bind_XorShift128.cpp)

pybind11_add_module(hgspy ${bindingSources})
target_link_libraries(hgspy PRIVATE ${hgs_libs})
