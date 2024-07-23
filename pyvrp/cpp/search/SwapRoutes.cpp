#include "SwapRoutes.h"

using pyvrp::Cost;
using pyvrp::search::SwapRoutes;

Cost SwapRoutes::evaluate(Route *U,
                          Route *V,
                          CostEvaluator const &costEvaluator)
{
    if (U == V || U->vehicleType() == V->vehicleType())
        return 0;

    return op.evaluate((*U)[0], (*V)[0], costEvaluator);
}

void SwapRoutes::apply(Route *U, Route *V) const { op.apply((*U)[0], (*V)[0]); }

SwapRoutes::SwapRoutes(ProblemData const &data)
    : LocalSearchOperator<Route>(data), op(data)
{
}
