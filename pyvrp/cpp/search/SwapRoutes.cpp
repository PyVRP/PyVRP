#include "SwapRoutes.h"

using pyvrp::Cost;
using pyvrp::search::SwapRoutes;

Cost SwapRoutes::evaluate(Route *U,
                          Route *V,
                          CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (U == V || U->vehicleType() == V->vehicleType())
        return 0;

    // Evaluate swapping the routes after the two depots.
    return op.evaluate((*U)[0], (*V)[0], costEvaluator);
}

void SwapRoutes::apply(Route *U, Route *V) const
{
    stats_.numApplications++;
    op.apply((*U)[0], (*V)[0]);
}

SwapRoutes::SwapRoutes(ProblemData const &data) : RouteOperator(data), op(data)
{
}

template <> bool pyvrp::search::supports<SwapRoutes>(ProblemData const &data)
{
    // Swapping routes has no benefit if all vehicles are the same.
    return data.numVehicleTypes() > 1;
}
