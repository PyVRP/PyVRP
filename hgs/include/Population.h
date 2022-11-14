#ifndef POPULATION_H
#define POPULATION_H

#include "Individual.h"
#include "Params.h"
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

        bool operator<(IndividualWrapper const &other) const
        {
            return indiv->cost() < other.indiv->cost();
        }
    };

public:
    using SubPopulation = std::vector<IndividualWrapper>;

private:
    using Parents = std::pair<Individual const *, Individual const *>;

    Params &params;    // Problem parameters
    XorShift128 &rng;  // Random number generator

    SubPopulation feasible;    // Sub-population ordered asc. by cost
    SubPopulation infeasible;  // Sub-population ordered asc. by cost

    Individual bestSol;

    // Evaluates the biased fitness of all individuals in the sub-population
    void updateBiasedFitness(SubPopulation &subPop) const;

    // Removes a duplicate individual from the sub-population if there exists
    // one. If there are multiple duplicate individuals, then the one with the
    // lowest index in the sub-population is removed first.
    static bool removeDuplicate(SubPopulation &subPop);

    // Removes the worst individual in terms of biased fitness
    static void removeWorstBiasedFitness(SubPopulation &subPop);

    // Generates a population of passed-in size
    void generatePopulation(size_t popSize);

    // Selects an individual by binary tournament
    Individual const *getBinaryTournament();

public:
    // Add an individual in the population. Survivor selection is automatically
    // triggered whenever the population reaches its maximum size.
    void addIndividual(Individual const &indiv);

    // Cleans all solutions and generates a new initial population (only used
    // when running HGS until a time limit, in which case the algorithm restarts
    // until the time limit is reached)
    void restart();

    /**
     * Re-orders the population by cost.
     */
    void reorder()
    {
        auto const op = [](auto const &wrapper1, auto const &wrapper2) {
            return wrapper1.indiv->cost() < wrapper2.indiv->cost();
        };
        std::sort(feasible.begin(), feasible.end(), op);
        std::sort(infeasible.begin(), infeasible.end(), op);
    }

    // Selects two (if possible non-identical) parents by binary tournament
    Parents selectParents();

    /**
     * Returns the best feasible solution that was observed during iteration.
     */
    [[nodiscard]] Individual const &getBestFound() const { return bestSol; }

    /**
     * Returns the current best objective value in the feasible sub-population
     * or ``INT_MAX`` if no feasible solution exists.
     */
    [[nodiscard]] size_t getCurrentBestFeasibleCost() const
    {
        return !feasible.empty() ? feasible[0].indiv->cost() : INT_MAX;
    }

    Population(Params &params, XorShift128 &rng);
};

#endif
