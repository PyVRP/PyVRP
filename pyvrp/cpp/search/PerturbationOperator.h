#ifndef PYVRP_SEARCH_PERTURBATIONOPERATOR_H
#define PYVRP_SEARCH_PERTURBATIONOPERATOR_H

#include "CostEvaluator.h"
#include "DynamicBitset.h"
#include "Route.h"

#include <vector>

namespace pyvrp::search
{
/**
 * Base class for perturbation operators. These operators modify solutions,
 * typically by removing or moving around nodes.
 */
class PerturbationOperator
{
public:
    /**
     * Applies the perturbation operator to the given solution.
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
     *     Order of nodes to use when applying the perturbation operator.
     * promising
     *     Bitset that marks promising nodes for further evaluation.
     */
    virtual void operator()(std::vector<search::Route::Node> &nodes,
                            std::vector<search::Route> &routes,
                            CostEvaluator const &costEvaluator,
                            std::vector<std::vector<size_t>> const &neighbours,
                            std::vector<size_t> const &orderNodes,
                            DynamicBitset &promising)
        = 0;

    virtual ~PerturbationOperator() = default;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_PERTURBATIONOPERATOR_H
