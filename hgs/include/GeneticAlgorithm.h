#ifndef GENETIC_H
#define GENETIC_H

#include "Individual.h"
#include "LocalSearch.h"
#include "Params.h"
#include "Population.h"
#include "Result.h"
#include "StoppingCriterion.h"
#include "XorShift128.h"

#include <functional>
#include <unordered_set>
#include <vector>

// Class to run the genetic algorithm, which incorporates functionality of
// population management, doing crossovers and updating parameters.
class GeneticAlgorithm
{
    using xOp = std::function<Individual(
        std::pair<Individual const *, Individual const *> const &,
        Params const &,
        XorShift128 &)>;

    Params &params;    // Problem parameters
    XorShift128 &rng;  // Random number generator
    Population &population;
    LocalSearch &localSearch;

    std::vector<bool> loadFeas;  // load feasibility of recent individuals
    std::vector<bool> timeFeas;  // time feasibility of recent individuals

    std::vector<xOp> operators;  // crossover operators

    /**
     * Runs the crossover algorithm: each given crossover operator is applied
     * once, its resulting offspring inspected, and a geometric acceptance
     * criterion is applied to select the offspring to return.
     */
    [[nodiscard]] Individual crossover() const;

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
     * Add a crossover operator to the genetic search algorithm.
     */
    void addCrossoverOperator(xOp const &op) { operators.push_back(op); }

    /**
     * Runs the genetic algorithm with the given stopping criterion.
     *
     * @param stop The stopping criterion to use.
     * @return     Result object containing the best solution, and some optional
     *             statistics.
     */
    Result run(StoppingCriterion &stop);

    GeneticAlgorithm(Params &params,
                     XorShift128 &rng,
                     Population &population,
                     LocalSearch &localSearch);
};

#endif
