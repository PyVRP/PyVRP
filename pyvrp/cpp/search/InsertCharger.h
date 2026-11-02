#ifndef PYVRP_SEARCH_INSERTCHARGER_H
#define PYVRP_SEARCH_INSERTCHARGER_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * InsertCharger(data: ProblemData)
 *
 * Evaluates inserting a charger directly before or after a node :math:`U`,
 * for each charger. This operator only considers routes with excess charge.
 */
class InsertCharger : public UnaryOperator
{
    using UnaryOperator::UnaryOperator;

    struct Move
    {
        Cost cost = 0;
        size_t charger = 0;
        size_t pos = 0;  // position in the route to insert the charger at
    };

    Move move_;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U) const override;

    std::string name() const override;

    static bool supports(ProblemData const &data);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_INSERTCHARGER_H
