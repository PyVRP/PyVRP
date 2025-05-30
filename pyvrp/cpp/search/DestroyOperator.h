#ifndef PYVRP_SEARCH_DESTROYOPERATOR_H
#define PYVRP_SEARCH_DESTROYOPERATOR_H

#include "CostEvaluator.h"
#include "RandomNumberGenerator.h"
#include "Route.h"

#include <vector>

namespace pyvrp::search
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
     */
    virtual void operator()(std::vector<search::Route::Node> &nodes,
                            std::vector<search::Route> &routes,
                            CostEvaluator const &costEvaluator,
                            std::vector<std::vector<size_t>> const &neighbours)
        = 0;

    virtual ~DestroyOperator() = default;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_DESTROYOPERATOR_H
