#include "SwapRoutes.h"

using pyvrp::Cost;
using pyvrp::search::Route;
using pyvrp::search::SwapRoutes;
using TWS = pyvrp::TimeWindowSegment;

Cost SwapRoutes::evaluate([[maybe_unused]] Route *U,
                          [[maybe_unused]] Route *V,
                          [[maybe_unused]] CostEvaluator const &costEvaluator)
{
    // TODO
    return 0;
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
