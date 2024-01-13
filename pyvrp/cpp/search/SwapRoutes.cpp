#include "SwapRoutes.h"

#include <vector>

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

    // Evaluate swapping the routes after the two depots using 2-OPT.
    return opt.evaluate((*U)[0], (*V)[0], costEvaluator);
}

void SwapRoutes::apply(Route *U, Route *V) const
{
    opt.apply((*U)[0], (*V)[0]);
}

SwapRoutes::SwapRoutes(ProblemData const &data)
    : LocalSearchOperator<Route>(data), opt(data)
{
}
