#ifndef PYVRP_SEARCH_REMOVEADJACENTDEPOT_H
#define PYVRP_SEARCH_REMOVEADJACENTDEPOT_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RemoveAdjacentDepot(data: ProblemData)
 *
 * Evaluates removing an adjacent reload depot of a node :math:`U`.
 */
class RemoveAdjacentDepot : public UnaryOperator
{
    using UnaryOperator::UnaryOperator;

    enum class MoveType
    {
        REMOVE_PREV,
        REMOVE_NEXT,
    };

    MoveType move_;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U) const override;
};

template <> bool supports<RemoveAdjacentDepot>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_REMOVEADJACENTDEPOT_H
