cmake_minimum_required(VERSION 3.15)

set(bindingSources
        bindings.cpp
        bind_crossover.cpp
        bind_diversity.cpp
        bind_Exchange.cpp
        bind_Individual.cpp
        bind_LocalSearch.cpp
        bind_LocalSearchOperator.cpp
        bind_LocalSearchParams.cpp
        bind_Matrix.cpp
        bind_MoveTwoClientsReversed.cpp
        bind_PenaltyManager.cpp
        bind_PenaltyParams.cpp
        bind_ProblemData.cpp
        bind_RelocateStar.cpp
        bind_SwapStar.cpp
        bind_TimeWindowSegment.cpp
        bind_TwoOpt.cpp
        bind_XorShift128.cpp)

pybind11_add_module(hgspy ${bindingSources})
target_link_libraries(hgspy PRIVATE ${hgs_libs})

install(TARGETS hgspy DESTINATION ${CMAKE_INSTALL_PREFIX})
