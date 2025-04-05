#include "TripRelocate.h"

#include "Route.h"

#include <cassert>

using pyvrp::search::TripRelocate;

pyvrp::Cost
TripRelocate::evaluate(Route::Node *U,
                       Route::Node *V,
                       [[maybe_unused]] CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot() && !V->isEndDepot());

    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    if (vRoute->numTrips() == vRoute->maxTrips())
        // Then we cannot insert another depot in V so we have nothing to do.
        return 0;

    Cost deltaCost = 0;

    if (uRoute->numClients() == 1)
        deltaCost -= uRoute->fixedVehicleCost();

    if (vRoute->empty())
        deltaCost += vRoute->fixedVehicleCost();

    if (p(U)->isReloadDepot() || n(U)->isReloadDepot())
    {
        // TODO U borders reload depot, test moving it along with U.
        // TODO test if reload depot is allowed in V
        return 0;
    }

    if (!V->isReloadDepot() && !n(V)->isReloadDepot())
    {
        // Test inserting U with a reload depot
        return 0;
    }

    return 0;
}

void TripRelocate::apply([[maybe_unused]] Route::Node *U,
                         [[maybe_unused]] Route::Node *V) const
{
    // TODO
}
