#include "SwapTails.h"

#include "Route.h"

#include <cassert>

using pyvrp::search::SwapTails;

namespace
{
bool onLastTrip(pyvrp::search::Route::Node *node)
{
    auto const *route = node->route();
    return node->trip() + 1 == route->numTrips();
}
}  // namespace

pyvrp::Cost SwapTails::evaluate(Route::Node *U,
                                Route::Node *V,
                                CostEvaluator const &costEvaluator)
{
    assert(!U->isEndDepot() && !U->isReloadDepot());
    assert(!V->isEndDepot() && !V->isReloadDepot());

    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    if (uRoute == vRoute)
        return 0;  // same route

    if (uRoute->idx() > vRoute->idx() && !uRoute->empty() && !vRoute->empty())
        return 0;  // move will be tackled in a later iteration

    if (!onLastTrip(U) || !onLastTrip(V))
        // We cannot move reload depots, so we only evaluate a move if it does
        // not include a reload depot.
        return 0;

    Cost deltaCost = 0;

    // We're going to incur fixed cost if a route is currently empty but
    // becomes non-empty due to the proposed move.
    if (uRoute->empty() && !n(V)->isEndDepot())
        deltaCost += uRoute->fixedVehicleCost();

    if (vRoute->empty() && !n(U)->isEndDepot())
        deltaCost += vRoute->fixedVehicleCost();

    // We lose fixed cost if a route becomes empty due to the proposed move.
    if (!uRoute->empty() && U->isStartDepot() && n(V)->isEndDepot())
        deltaCost -= uRoute->fixedVehicleCost();

    if (!vRoute->empty() && V->isStartDepot() && n(U)->isEndDepot())
        deltaCost -= vRoute->fixedVehicleCost();

    if (!n(U)->isEndDepot() && !n(V)->isEndDepot())
    {
        auto const uProposal
            = Route::Proposal(uRoute->before(U->idx()),
                              vRoute->between(V->idx() + 1, vRoute->size() - 2),
                              uRoute->at(uRoute->size() - 1));

        auto const vProposal
            = Route::Proposal(vRoute->before(V->idx()),
                              uRoute->between(U->idx() + 1, uRoute->size() - 2),
                              vRoute->at(vRoute->size() - 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else if (!n(U)->isEndDepot() && n(V)->isEndDepot())
    {
        auto const uProposal = Route::Proposal(uRoute->before(U->idx()),
                                               uRoute->at(uRoute->size() - 1));

        auto const vProposal
            = Route::Proposal(vRoute->before(V->idx()),
                              uRoute->between(U->idx() + 1, uRoute->size() - 2),
                              vRoute->at(vRoute->size() - 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else if (n(U)->isEndDepot() && !n(V)->isEndDepot())
    {
        auto const uProposal
            = Route::Proposal(uRoute->before(U->idx()),
                              vRoute->between(V->idx() + 1, vRoute->size() - 2),
                              uRoute->at(uRoute->size() - 1));

        auto const vProposal = Route::Proposal(vRoute->before(V->idx()),
                                               vRoute->at(vRoute->size() - 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }

    return deltaCost;
}

void SwapTails::apply(Route::Node *U, Route::Node *V) const
{
    auto *nU = n(U);
    auto *nV = n(V);

    auto insertIdx = U->idx() + 1;
    while (!nV->isEndDepot())
    {
        auto *node = nV;
        nV = n(nV);
        V->route()->remove(node->idx());
        U->route()->insert(insertIdx++, node);
    }

    insertIdx = V->idx() + 1;
    while (!nU->isEndDepot())
    {
        auto *node = nU;
        nU = n(nU);
        U->route()->remove(node->idx());
        V->route()->insert(insertIdx++, node);
    }
}
