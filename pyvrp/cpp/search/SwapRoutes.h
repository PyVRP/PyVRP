#ifndef PYVRP_SEARCH_SWAPROUTES_H
#define PYVRP_SEARCH_SWAPROUTES_H

#include "LocalSearchOperator.h"
#include "SwapTails.h"

namespace pyvrp::search
{
/**
 * SwapRoutes(data: ProblemData)
 *
 * Swaps the complete routes containing :math:`U` and :math:`V`. Nonempty
 * routes are represented by their first client. Empty routes are represented
 * by their start depot.
 */
class SwapRoutes : public BinaryOperator
{
    SwapTails swapTails_;

public:
    explicit SwapRoutes(ProblemData const &data);

    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;

    void init(Solution &solution) override;

    std::string name() const override;
};

template <> bool supports<SwapRoutes>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SWAPROUTES_H
