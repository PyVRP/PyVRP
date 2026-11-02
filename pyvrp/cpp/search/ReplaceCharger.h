#ifndef PYVRP_SEARCH_REPLACECHARGER_H
#define PYVRP_SEARCH_REPLACECHARGER_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * ReplaceCharger(data: ProblemData)
 *
 * Evaluates replacing an adjacent charger of a node :math:`U` by a charger on
 * either side of :math:`U`, for each charger.
 */
class ReplaceCharger : public UnaryOperator
{
    using UnaryOperator::UnaryOperator;

    struct Move
    {
        Cost cost = 0;
        size_t charger = 0;
        size_t removePos = 0;  // position of the current charger
        size_t insertPos = 0;  // position of the new charger, after removal
    };

    Move move_;

    // Evaluates replacing the charger before U.
    void evaluatePrev(Route::Node *U, CostEvaluator const &costEvaluator);

    // Evaluates replacing the charger after U.
    void evaluateNext(Route::Node *U, CostEvaluator const &costEvaluator);

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U) const override;

    std::string name() const override;

    static bool supports(ProblemData const &data);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_REPLACECHARGER_H
