#ifndef PYVRP_RELOCATESTAR_H
#define PYVRP_RELOCATESTAR_H

#include "Exchange.h"
#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RelocateStar(data: ProblemData)
 *
 * Performs the best :math:`(1, 0)`-exchange move between routes :math:`U` and
 * :math:`V`.
 *
 * .. note::
 *
 *    See the :class:`Exchange10` node operator for details.
 */
class RelocateStar : public LocalSearchOperator<Route>
{
    struct Move
    {
        Cost deltaCost = 0;
        Route::Node *from = nullptr;
        Route::Node *to = nullptr;
    };

    Exchange<1, 0> relocate;
    Move move;

public:
    Cost
    evaluate(Route *U, Route *V, CostEvaluator const &costEvaluator) override;

    void apply(Route *U, Route *V) const override;

    RelocateStar(ProblemData const &data)
        : LocalSearchOperator<Route>(data), relocate(data)
    {
    }
};
}  // namespace pyvrp::search

#endif  // PYVRP_RELOCATESTAR_H
