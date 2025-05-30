#ifndef PYVRP_PERTURB_REPAIROPERATOR_H
#define PYVRP_PERTURB_REPAIROPERATOR_H

#include "CostEvaluator.h"
#include "RandomNumberGenerator.h"
#include "search/Route.h"

#include <vector>

namespace pyvrp::perturb
{
/**
 * Base class for repair operators used in the DestroyRepair class. These
 * operators modify solutions by inserting unplanned clients into routes.
 */
class RepairOperator
{
public:
    /**
     * Applies the repair operator to the given solution.
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

    virtual ~RepairOperator() = default;
};
}  // namespace pyvrp::perturb

#endif  // PYVRP_PERTURB_REPAIROPERATOR_H
