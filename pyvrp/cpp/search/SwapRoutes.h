#ifndef PYVRP_SEARCH_SWAPROUTES_H
#define PYVRP_SEARCH_SWAPROUTES_H

#include "LocalSearchOperator.h"
#include "Measure.h"

namespace pyvrp::search
{
/**
 * SwapRoutes(data: ProblemData)
 *
 * This operator evaluates exchanging the visits of two routes :math:`U` and
 * :math:`V`.
 */
class SwapRoutes : public LocalSearchOperator<Route>
{
public:
    Cost
    evaluate(Route *U, Route *V, CostEvaluator const &costEvaluator) override;

    void apply(Route *U, Route *V) const override;

    explicit SwapRoutes(ProblemData const &data);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SWAPROUTES_H
