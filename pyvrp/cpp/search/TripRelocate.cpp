#include "TripRelocate.h"

#include "Route.h"

#include <cassert>

using pyvrp::search::TripRelocate;

namespace
{
class ReloadDepotSegment
{
    // TODO
};
}  // namespace

pyvrp::Cost
TripRelocate::evaluate(Route::Node *U,
                       Route::Node *V,
                       [[maybe_unused]] CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot() && !V->isEndDepot());

    [[maybe_unused]] auto const *uRoute = U->route();
    auto const *vRoute = V->route();
    assert(uRoute && vRoute);

    if (p(U)->isReloadDepot() || n(U)->isReloadDepot())
    {
        // TODO U borders reload depot, test removing it.
    }

    if (!V->isReloadDepot() && !n(V)->isReloadDepot())
    {
        if (vRoute->numTrips() == vRoute->maxTrips())
            // Then we cannot do anything more than evaluate removing the
            // reload depot from U, since we cannot insert another depot in V.
            return 0;

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
