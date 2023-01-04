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

    ProblemData const &data;  // Problem data
    XorShift128 &rng;         // Random number generator

    SubPopulation feasible;    // Sub-population of feasible individuals
    SubPopulation infeasible;  // Sub-population of infeasible individuals

    Individual bestSol;

    // Evaluates the biased fitness of all individuals in the sub-population
    void updateBiasedFitness(SubPopulation &subPop) const;

    // Performs survivor selection: individuals in the given sub-population are
    // purged until the population is reduced to the ``minPopSize``. Purging
    // happens first to duplicate solutions, and then to solutions with high
    // biased fitness.
    void survivorSelection(SubPopulation &subPop);

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
     * @return Total size of the current population.
     */
    size_t size() const;

    /**
     * Constructs a population with ``minPopSize`` random individuals. This
     * includes a random, possibly infeasible, initial best solution.
     *
     * @param data Data instance describing the problem that is being solved.
     * @param rng  Random number generator.
     */
    Population(ProblemData const &data, XorShift128 &rng);
};

#endif
