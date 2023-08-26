#ifndef PYVRP_SEARCH_PRIMITIVES_H
#define PYVRP_SEARCH_PRIMITIVES_H

#include "CostEvaluator.h"
#include "Measure.h"
#include "Route.h"

namespace pyvrp::search
{
// Evaluates inserting U after V.
Cost insertCost(Route::Node *U,
                Route::Node *V,
                ProblemData const &data,
                CostEvaluator const &costEvaluator);

// Evaluates removing U.
Cost removeCost(Route::Node *U,
                ProblemData const &data,
                CostEvaluator const &costEvaluator);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_PRIMITIVES_H
