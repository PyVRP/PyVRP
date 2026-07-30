#include "RelocateTail.h"

#include "Route.h"
#include "SwapTails.h"

#include <cassert>

using pyvrp::search::RelocateTail;

std::pair<pyvrp::Cost, bool> RelocateTail::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;
    assert(!U->isDepot());
    assert(!V->isEndDepot() && !V->isReloadDepot());

    if (!U->route() || U->route() == V->route() || p(U)->isDepot())
        return std::make_pair(0, false);

    auto *vRoute = V->route();
    auto *beforeEnd = (*vRoute)[vRoute->size() - 2];
    assert(n(beforeEnd)->isEndDepot());

    if (beforeEnd->isReloadDepot())
        return std::make_pair(0, false);

    auto op = SwapTails(data);
    return op.evaluate(p(U), beforeEnd, costEvaluator);
}

void RelocateTail::apply(Route::Node *U, Route::Node *V) const
{
    stats_.numApplications++;

    auto *vRoute = V->route();
    auto *beforeEnd = (*vRoute)[vRoute->size() - 2];
    assert(n(beforeEnd)->isEndDepot());

    auto const op = SwapTails(data);
    op.apply(p(U), beforeEnd);
}

std::string RelocateTail::name() const { return "RelocateTail"; }

bool RelocateTail::supports(ProblemData const &data)
{
    // Does not work for TSP, since the operator needs two routes.
    return data.numVehicles() > 1;
}
