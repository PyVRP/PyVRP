#ifndef PYVRP_SUBPOPULATION_H
#define PYVRP_SUBPOPULATION_H

#include "CostEvaluator.h"
#include "Solution.h"
#include "diversity/diversity.h"

#include <functional>
#include <iosfwd>
#include <stdexcept>
#include <vector>

namespace pyvrp
{
struct PopulationParams
{
    size_t minPopSize;
    size_t generationSize;
    size_t nbElite;
    size_t nbClose;
    double lbDiversity;
    double ubDiversity;

    PopulationParams(size_t minPopSize = 25,
                     size_t generationSize = 40,
                     size_t nbElite = 4,
                     size_t nbClose = 5,
                     double lbDiversity = 0.1,
                     double ubDiversity = 0.5)
        : minPopSize(minPopSize),
          generationSize(generationSize),
          nbElite(nbElite),
          nbClose(nbClose),
          lbDiversity(lbDiversity),
          ubDiversity(ubDiversity)
    {
        if (lbDiversity < 0 || lbDiversity > 1)
            throw std::invalid_argument("lb_diversity must be in [0, 1].");

        if (ubDiversity < 0 || ubDiversity > 1)
            throw std::invalid_argument("ub_diversity must be in [0, 1].");

        if (ubDiversity <= lbDiversity)
        {
            auto const msg = "ub_diversity <= lb_diversity not understood.";
            throw std::invalid_argument(msg);
        }
    }

    size_t maxPopSize() const { return minPopSize + generationSize; }
};

/**
 * SubPopulation(diversity_op: DiversityMeasure, params: PopulationParams)
 *
 * Creates a SubPopulation instance.
 *
 * This subpopulation manages a collection of solutions, and initiates
 * survivor selection (purging) when their number grows large. A
 * subpopulation's solutions can be accessed via indexing and iteration.
 * Each solution is stored as a tuple of type ``_Item``, which stores
 * the solution itself, a fitness score (higher is worse), and a list
 * of proximity values to the other solutions in the subpopulation.
 *
 * Parameters
 * ----------
 * diversity_op
 *     Operator to use to determine pairwise diversity between solutions.
 * params
 *     Population parameters.
 */
class SubPopulation
{
    diversity::DiversityMeasure divOp;
    PopulationParams const &params;  // owned by Population, on the Python side

public:
    struct Item
    {
        using Proximity = std::vector<std::pair<double, Solution const *>>;

        PopulationParams const *params;

        // Note that this pointer is not owned by the Item - it is merely a
        // reference to memory owned and allocated by the SubPopulation this
        // item is part of. The SubPopulation remains responsible for managing
        // that memory.
        Solution const *solution;

        // Fitness should be used carefully: only directly after updateFitness
        // was called. At any other moment, it will be outdated.
        double fitness;
        Proximity proximity;

        double avgDistanceClosest() const;
    };

private:
    std::vector<Item> items;

    // Removes the element at the given iterator location from the items.
    void remove(std::vector<Item>::iterator const &iterator);

public:
    SubPopulation(diversity::DiversityMeasure divOp,
                  PopulationParams const &params);

    ~SubPopulation();

    /**
     * Adds the given solution to the subpopulation. Survivor selection is
     * automatically triggered when the population reaches its maximum size.
     *
     * Parameters
     * ----------
     * solution
     *     Solution to add to the subpopulation.
     * cost_evaluator
     *     CostEvaluator to use to compute the cost.
     */
    void add(Solution const *solution, CostEvaluator const &costEvaluator);

    std::vector<Item>::const_iterator cbegin() const;

    std::vector<Item>::const_iterator cend() const;

    size_t size() const;

    Item const &operator[](size_t idx) const;

    /**
     * Performs survivor selection: solutions in the subpopulation are
     * purged until the population is reduced to the ``min_pop_size``.
     * Purging happens to duplicate solutions first, and then to solutions
     * with high biased fitness.
     *
     * Parameters
     * ----------
     * cost_evaluator
     *     CostEvaluator to use to compute the cost.
     */
    void purge(CostEvaluator const &costEvaluator);

    /**
     * Updates the biased fitness scores of solutions in the subpopulation.
     * This fitness depends on the quality of the solution (based on its cost)
     * and the diversity w.r.t. to other solutions in the subpopulation.
     *
     * .. warning::
     *
     *    This function must be called before accessing the
     *    :meth:`~SubPopulationItem.fitness` attribute.
     */
    void updateFitness(CostEvaluator const &costEvaluator);
};
}  // namespace pyvrp

#endif  // PYVRP_SUBPOPULATION_H
