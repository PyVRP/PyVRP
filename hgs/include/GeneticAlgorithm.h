#ifndef GENETIC_H
#define GENETIC_H

#include "Individual.h"
#include "LocalSearch.h"
#include "PenaltyManager.h"
#include "Population.h"
#include "ProblemData.h"
#include "Result.h"
#include "SolverParams.h"
#include "StoppingCriterion.h"
#include "XorShift128.h"
#include "crossover.h"

#include <unordered_set>
#include <vector>

// Class to run the genetic algorithm, which incorporates functionality of
// population management, doing crossovers and updating parameters.
class GeneticAlgorithm
{
    ProblemData &data;
    PenaltyManager &penaltyManager;
    XorShift128 &rng;
    Population &population;
    LocalSearch &localSearch;
    CrossoverOperator crossover;
    SolverParams params;

    std::vector<bool> loadFeas;  // load feasibility of recent individuals
    std::vector<bool> timeFeas;  // time feasibility of recent individuals

    /**
     * Performs local search and adds the individual to the population. If the
     * individual is infeasible, with some probability we try to repair it and
     * add it if this succeeds.
     */
    void educate(Individual &indiv);

    /**
     * Updates the infeasibility penalties, based on the feasibility status of
     * the most recent individuals.
     */
    void updatePenalties();

public:
    /**
     * Runs the genetic algorithm with the given stopping criterion.
     *
     * @param stop The stopping criterion to use.
     * @return     Result object containing the best solution, and some optional
     *             statistics.
     */
    Result run(StoppingCriterion &stop);

    GeneticAlgorithm(ProblemData &data,
                     PenaltyManager &penaltyManager,
                     XorShift128 &rng,
                     Population &population,
                     LocalSearch &localSearch,
                     CrossoverOperator op,
                     SolverParams params = SolverParams());
};

#endif
