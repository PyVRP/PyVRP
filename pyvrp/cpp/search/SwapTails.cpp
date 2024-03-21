#include "SwapTails.h"

#include "Route.h"

#include <cassert>

using pyvrp::search::SwapTails;

pyvrp::Cost SwapTails::evaluate(Route::Node *U,
                                Route::Node *V,
                                CostEvaluator const &costEvaluator)
{
    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    if (uRoute == vRoute || uRoute->idx() > vRoute->idx())
        return 0;  // same route, or move will be tackled in a later iteration

    Cost deltaCost = 0;

    // We're going to incur fixed cost if a route is currently empty but
    // becomes non-empty due to the proposed move.
    if (uRoute->empty() && U->isDepot() && !n(V)->isDepot())
        deltaCost += uRoute->fixedVehicleCost();

    if (vRoute->empty() && V->isDepot() && !n(U)->isDepot())
        deltaCost += vRoute->fixedVehicleCost();

    // We lose fixed cost if a route becomes empty due to the proposed move.
    if (!uRoute->empty() && U->isDepot() && n(V)->isDepot())
        deltaCost -= uRoute->fixedVehicleCost();

    if (!vRoute->empty() && V->isDepot() && n(U)->isDepot())
        deltaCost -= vRoute->fixedVehicleCost();

    if (U->idx() < uRoute->size() && V->idx() < vRoute->size())
    {
        auto const uProposal
            = uRoute->proposal(uRoute->before(U->idx()),
                               vRoute->between(V->idx() + 1, vRoute->size()),
                               uRoute->at(uRoute->size() + 1));

        auto const vProposal
            = vRoute->proposal(vRoute->before(V->idx()),
                               uRoute->between(U->idx() + 1, uRoute->size()),
                               vRoute->at(vRoute->size() + 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else if (U->idx() < uRoute->size() && V->idx() >= vRoute->size())
    {
        auto const uProposal = uRoute->proposal(uRoute->before(U->idx()),
                                                uRoute->at(uRoute->size() + 1));

        auto const vProposal
            = vRoute->proposal(vRoute->before(V->idx()),
                               uRoute->between(U->idx() + 1, uRoute->size()),
                               vRoute->at(vRoute->size() + 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else if (U->idx() >= uRoute->size() && V->idx() < vRoute->size())
    {
        auto const uProposal
            = uRoute->proposal(uRoute->before(U->idx()),
                               vRoute->between(V->idx() + 1, vRoute->size()),
                               uRoute->at(uRoute->size() + 1));

        auto const vProposal = vRoute->proposal(vRoute->before(V->idx()),
                                                vRoute->at(vRoute->size() + 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }

    return deltaCost;
}

void SwapTails::apply(Route::Node *U, Route::Node *V) const
{
    auto *nU = n(U);
    auto *nV = n(V);

    auto insertIdx = U->idx() + 1;
    while (!nV->isDepot())
    {
        auto *node = nV;
        nV = n(nV);
        V->route()->remove(node->idx());
        U->route()->insert(insertIdx++, node);
    }

    insertIdx = V->idx() + 1;
    while (!nU->isDepot())
    {
        auto *node = nU;
        nU = n(nU);
        U->route()->remove(node->idx());
        V->route()->insert(insertIdx++, node);
    }
}
