#ifndef PYVRP_SEARCH_DESTROYOPERATOR_H
#define PYVRP_SEARCH_DESTROYOPERATOR_H

#include "CostEvaluator.h"
#include "RandomNumberGenerator.h"
#include "Route.h"

#include <vector>

namespace pyvrp::search
{
/**
 * Base class for destroy operators used in the LocalSearch class. These
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
     *     List of search routes used in the solution.
     * cost_evaluator
     *     Cost evaluator to use.
     * neighbours
     *     List of lists that defines the local search neighbourhood.
     * order_nodes
     *     Order of nodes to use when applying the destroy operator.
     */
    virtual void operator()(std::vector<search::Route::Node> &nodes,
                            std::vector<search::Route> &routes,
                            CostEvaluator const &costEvaluator,
                            std::vector<std::vector<size_t>> const &neighbours,
                            std::vector<size_t> const &orderNodes)
        = 0;

    virtual ~DestroyOperator() = default;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_DESTROYOPERATOR_H
