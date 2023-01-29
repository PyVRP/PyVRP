#ifndef HGS_BINDINGS_H
#define HGS_BINDINGS_H

#include <pybind11/functional.h>  // needed for modular function operators
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // needed for automatic conversions

// General classes
void bind_GeneticAlgorithm(pybind11::module_ &m);
void bind_GeneticAlgorithmParams(pybind11::module_ &m);
void bind_Individual(pybind11::module_ &m);
void bind_LocalSearch(pybind11::module_ &m);
void bind_LocalSearchParams(pybind11::module_ &m);
void bind_Matrix(pybind11::module_ &m);
void bind_PenaltyManager(pybind11::module_ &m);
void bind_PenaltyParams(pybind11::module_ &m);
void bind_Population(pybind11::module_ &m);
void bind_PopulationParams(pybind11::module_ &m);
void bind_ProblemData(pybind11::module_ &m);
void bind_Result(pybind11::module_ &m);
void bind_Statistics(pybind11::module_ &m);
void bind_TimeWindowSegment(pybind11::module_ &m);
void bind_XorShift128(pybind11::module_ &m);

// Crossover operators
void bind_crossover(pybind11::module_ &m);

// Diversity measures
void bind_diversity(pybind11::module_ &m);

// Stopping criteria
void bind_MaxIterations(pybind11::module_ &m);
void bind_MaxRuntime(pybind11::module_ &m);
void bind_NoImprovement(pybind11::module_ &m);
void bind_StoppingCriterion(pybind11::module_ &m);
void bind_TimedNoImprovement(pybind11::module_ &m);

// Local search operators
void bind_Exchange(pybind11::module_ &m);
void bind_LocalSearchOperator(pybind11::module_ &m);
void bind_MoveTwoClientsReversed(pybind11::module_ &m);
void bind_RelocateStar(pybind11::module_ &m);
void bind_SwapStar(pybind11::module_ &m);
void bind_TwoOpt(pybind11::module_ &m);

#endif  // HGS_BINDINGS_H
