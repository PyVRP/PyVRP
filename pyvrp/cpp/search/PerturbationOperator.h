#ifndef PYVRP_SEARCH_PERTURBATIONOPERATOR_H
#define PYVRP_SEARCH_PERTURBATIONOPERATOR_H

#include "CostEvaluator.h"
#include "DynamicBitset.h"
#include "LocalSearchOperator.h"
#include "Route.h"

#include <vector>

namespace pyvrp::search
{
/**
 * Context structure that holds all parameters needed by perturbation operators.
 */
struct PerturbationContext
{
    std::vector<search::Route::Node> &nodes;
    std::vector<search::Route> &routes;
    CostEvaluator const &costEvaluator;
    std::vector<std::vector<size_t>> const &neighbours;
    std::vector<size_t> const &orderNodes;
    DynamicBitset &promising;
    size_t const numPerturbations;
};

/**
 * Base class for perturbation operators. These operators modify solutions,
 * typically by removing or moving around nodes.
 */
class PerturbationOperator
{
public:
    /**
     * Applies the perturbation operator to the given solution.
     */
    virtual void operator()(PerturbationContext const &context) = 0;

    virtual ~PerturbationOperator() = default;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_PERTURBATIONOPERATOR_H
