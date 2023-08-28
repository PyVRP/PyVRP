#include "SwapRoutes.h"

using pyvrp::Cost;
using pyvrp::search::Route;
using pyvrp::search::SwapRoutes;
using TWS = pyvrp::TimeWindowSegment;

Cost SwapRoutes::evaluate(Route *U,
                          Route *V,
                          CostEvaluator const &costEvaluator)
{
    if (U->vehicleType() == V->vehicleType() || U->empty() || V->empty())
        return 0;

    Cost deltaCost = 0;

    // Load changes
    deltaCost += costEvaluator.loadPenalty(U->load(), V->capacity());
    deltaCost -= costEvaluator.loadPenalty(U->load(), U->capacity());

    deltaCost += costEvaluator.loadPenalty(V->load(), U->capacity());
    deltaCost -= costEvaluator.loadPenalty(V->load(), V->capacity());

    // TODO depot distance, time windows

    return deltaCost;
}

void SwapRoutes::apply([[maybe_unused]] Route *U,
                       [[maybe_unused]] Route *V) const
{
    // TODO
}

SwapRoutes::SwapRoutes(ProblemData const &data)
    : LocalSearchOperator<Route>(data)
{
}
