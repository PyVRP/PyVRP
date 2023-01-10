#ifndef POPULATION_H
#define POPULATION_H

#include "Individual.h"
#include "ProblemData.h"
#include "XorShift128.h"
#include "diversity.h"

#include <memory>
#include <unordered_map>
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

    DiversityMeasure divOp;

    SubPopulation feasible;    // Sub-population of feasible individuals
    SubPopulation infeasible;  // Sub-population of infeasible individuals

    Individual bestSol;

    // Proximity structure, from a given individual to other individuals
    // TODO can we do this without a hash map?
    using ProximityKey = Individual const *;
    using ProximityValue = std::vector<std::pair<int, Individual const *>>;
    std::unordered_map<ProximityKey, ProximityValue> proximity;

    // Evaluates the biased fitness of all individuals in the sub-population
    void updateBiasedFitness(SubPopulation &subPop) const;

    // Performs survivor selection: individuals in the given sub-population are
    // purged until the population is reduced to the ``minPopSize``. Purging
    // happens first to duplicate solutions, and then to solutions with high
    // biased fitness.
    void purge(SubPopulation &subPop);

    /**
     * @return The average diversity distance of this individual to the
     *         individuals nearest to it.
     */
    [[nodiscard]] double avgDistanceClosest(Individual const &indiv) const;

    /**
     * Adds the two given individuals to each other's proximity structure.
     *
     * @param first  First individual.
     * @param second Second individual.
     */
    void registerNearbyIndividual(Individual *first, Individual *second);

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
     * @return Total number of feasible individuals in the current population.
     */
    size_t numFeasible() const;

    /**
     * @return Total number of infeasible individuals in the current population.
     */
    size_t numInfeasible() const;

    /**
     * Constructs a population with ``minPopSize`` random individuals. This
     * includes a random, possibly infeasible, initial best solution.
     *
     * @param data           Data instance describing the problem that is being
     *                       solved.
     * @param penaltyManager Penalty manager, used to generate individuals.
     * @param rng            Random number generator.
     * @param op             Diversity measure to use.
     */
    Population(ProblemData const &data,
               PenaltyManager const &penaltyManager,
               XorShift128 &rng,
               DiversityMeasure op);
};

#endif
