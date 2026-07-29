#ifndef PYVRP_SEARCH_RELOCATEALTERNATIVE_H
#define PYVRP_SEARCH_RELOCATEALTERNATIVE_H

#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * RelocateAlternative(data: ProblemData)
 *
 * Evaluates replacing mutually exclusive group member :math:`U` with another
 * member of the same group and relocating the replacement after :math:`V`.
 * Group members are evaluated in order until an improving move is found.
 */
class RelocateAlternative : public BinaryOperator
{
    using BinaryOperator::BinaryOperator;

    struct Move
    {
        Cost cost = 0;
        Route::Node *alternative = nullptr;
    };

    Move move_;
    Solution *solution_ = nullptr;

    // Evaluates relocation moves when U and V are in the same route.
    void evalSameRoute(Route::Node *U,
                       Route::Node *V,
                       CostEvaluator const &costEvaluator);

    // Evaluates relocation moves when U and V are in different routes.
    void evalDifferentRoutes(Route::Node *U,
                             Route::Node *V,
                             CostEvaluator const &costEvaluator);

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;

    void init(Solution &solution) override;

    std::string name() const override;

    static bool supports(ProblemData const &data);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_RELOCATEALTERNATIVE_H
