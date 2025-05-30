#ifndef PYVRP_PERTURB_DESTROYOPERATOR_H
#define PYVRP_PERTURB_DESTROYOPERATOR_H

#include "CostEvaluator.h"
#include "RandomNumberGenerator.h"
#include "search/Route.h"

#include <vector>

namespace pyvrp::perturb
{
/**
 * Base class for destroy operators used in the DestroyRepair class. These
 * operators modify solutions by removing clients from routes.
 */
class DestroyOperator
{
public:
    /**
     * Applies the destroy operator to the given solution.
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

    virtual ~DestroyOperator() = default;
};
}  // namespace pyvrp::perturb

#endif  // PYVRP_PERTURB_DESTROYOPERATOR_H
