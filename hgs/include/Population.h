#ifndef POPULATION_H
#define POPULATION_H

#include "Individual.h"
#include "ProblemData.h"
#include "Statistics.h"
#include "XorShift128.h"

#include <memory>
#include <vector>

// Class representing the population of a genetic algorithm with do binary
// tournaments, update fitness values, etc.
class Population
{
    friend class Statistics;  // used to collect population statistics

    struct IndividualWrapper
    {
        std::unique_ptr<Individual> indiv;
        double fitness;
    };

public:
    using SubPopulation = std::vector<IndividualWrapper>;

private:
    using Parents = std::pair<Individual const *, Individual const *>;

    ProblemData &data;  // Problem data
    XorShift128 &rng;   // Random number generator

    SubPopulation feasible;    // Sub-population of feasible individuals
    SubPopulation infeasible;  // Sub-population of infeasible individuals

    Individual bestSol;

    // Evaluates the biased fitness of all individuals in the sub-population
    void updateBiasedFitness(SubPopulation &subPop) const;

    // Removes a duplicate individual from the sub-population if there exists
    // one. If there are multiple duplicate individuals, then the one with the
    // lowest index in the sub-population is removed first.
    static bool removeDuplicate(SubPopulation &subPop);

    // Removes the worst individual in terms of biased fitness
    static void removeWorstBiasedFitness(SubPopulation &subPop);

    // Selects an individual by binary tournament
    Individual const *getBinaryTournament();

public:
    /**
     * Adds the given individual to the population. Survivor selection is
     * automatically triggered when the population reaches its maximum size.
     *
     * @param indiv Individual to add.
     */
    void add(Individual const &indiv);

    /**
     * Selects two (if possible non-identical) parents by binary tournament,
     * subject to a diversity restriction.
     *
     * @return A pair of individuals (parents).
     */
    Parents select();

    /**
     * @return The best observed feasible solution over all iterations.
     */
    [[nodiscard]] Individual const &getBestFound() const;

    /**
     * Constructs a population with ``minPopSize`` random individuals. This
     * includes a random, possibly infeasible, initial best solution.
     *
     * @param data Data instance describing the problem that is being solved.
     * @param rng  Random number generator.
     */
    Population(ProblemData &data, XorShift128 &rng);
};

#endif
