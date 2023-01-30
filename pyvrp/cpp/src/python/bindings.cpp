#include "bindings.h"

PYBIND11_MODULE(hgspy, m)
{
    // Binds the hgspy module. The binding is done per header file, in separate
    // functions called bind_<header name>.

    // General, top-level definitions
    bind_Matrix(m);
    bind_GeneticAlgorithm(m);
    bind_GeneticAlgorithmParams(m);
    bind_Individual(m);
    bind_LocalSearch(m);
    bind_LocalSearchParams(m);
    bind_PenaltyManager(m);
    bind_PenaltyParams(m);
    bind_Population(m);
    bind_PopulationParams(m);
    bind_ProblemData(m);
    bind_Result(m);
    bind_Statistics(m);
    bind_TimeWindowSegment(m);
    bind_XorShift128(m);

    // Submodule for crossover operators
    pybind11::module xOps = m.def_submodule("crossover");
    bind_crossover(xOps);

    // Submodule for diversity measures
    pybind11::module diversity = m.def_submodule("diversity");
    bind_diversity(diversity);

    // Submodule for stopping criteria
    pybind11::module stop = m.def_submodule("stop");
    bind_StoppingCriterion(stop);  // abstract base type first

    // Submodule for local search operators
    pybind11::module lsOps = m.def_submodule("operators");
    bind_LocalSearchOperator(lsOps);  // abstract base types first
    bind_Exchange(lsOps);
    bind_MoveTwoClientsReversed(lsOps);
    bind_RelocateStar(lsOps);
    bind_SwapStar(lsOps);
    bind_TwoOpt(lsOps);
}
