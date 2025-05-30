#ifndef PYVRP_PERTURB_DESTROYREPAIROPERATOR_H
#define PYVRP_PERTURB_DESTROYREPAIROPERATOR_H

#include "CostEvaluator.h"
#include "search/Route.h"

#include <vector>

namespace pyvrp::perturb
{
/**
 * Base class for destroy and repair operators used in the DestroyRepair
 * class. These operators modify solutions by removing or inserting clients.
 */
class DestroyRepairOperator
{
public:
    /**
     * Applies the operator to the given solution.
     *
     * Parameters
     * ----------
     * nodes
     *     List of search nodes used in the solution.
     * routes
     *     List of search routes representing the solution.
     * cost_evaluator
     *     CostEvaluator to use to compute the cost.
     * neighbours
     *    List of neighbours for each node, used to determine which nodes are
     *    close to each other.
     * rng
     *     Random number generator.
     */
    virtual void operator()(std::vector<search::Route::Node> &nodes,
                            std::vector<search::Route> &routes,
                            CostEvaluator const &costEvaluator,
                            std::vector<std::vector<size_t>> const &neighbours,
                            RandomNumberGenerator &rng)
        = 0;

    virtual ~DestroyRepairOperator() = default;
};
}  // namespace pyvrp::perturb

#endif  // PYVRP_PERTURB_DESTROYREPAIROPERATOR_H
