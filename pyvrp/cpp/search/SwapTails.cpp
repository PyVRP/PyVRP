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

    if (V->idx() < vRoute->size())
    {
        auto const uDist = DistanceSegment::merge(
            data.distanceMatrix(),
            uRoute->before(U->idx()),
            vRoute->between(V->idx() + 1, vRoute->size()),
            uRoute->at(uRoute->size() + 1));

        deltaCost += static_cast<Cost>(uDist.distance());
    }
    else
    {
        auto const uDist
            = DistanceSegment::merge(data.distanceMatrix(),
                                     uRoute->before(U->idx()),
                                     uRoute->at(uRoute->size() + 1));

        deltaCost += static_cast<Cost>(uDist.distance());
    }

    if (U->idx() < uRoute->size())
    {
        auto const vDist = DistanceSegment::merge(
            data.distanceMatrix(),
            vRoute->before(V->idx()),
            uRoute->between(U->idx() + 1, uRoute->size()),
            vRoute->at(vRoute->size() + 1));

        deltaCost += static_cast<Cost>(vDist.distance());
    }
    else
    {
        auto const vDist
            = DistanceSegment::merge(data.distanceMatrix(),
                                     vRoute->before(V->idx()),
                                     vRoute->at(vRoute->size() + 1));

        deltaCost += static_cast<Cost>(vDist.distance());
    }

    deltaCost -= static_cast<Cost>(uRoute->distance());
    deltaCost -= static_cast<Cost>(vRoute->distance());

    deltaCost -= costEvaluator.twPenalty(uRoute->timeWarp());
    deltaCost -= costEvaluator.twPenalty(vRoute->timeWarp());

    deltaCost -= costEvaluator.loadPenalty(uRoute->load(), uRoute->capacity());
    deltaCost -= costEvaluator.loadPenalty(vRoute->load(), vRoute->capacity());

    if (deltaCost >= 0)
        return deltaCost;

    if (V->idx() < vRoute->size())
    {
        auto const uDS = DurationSegment::merge(
            data.durationMatrix(),
            uRoute->before(U->idx()),
            vRoute->between(V->idx() + 1, vRoute->size()),
            uRoute->at(uRoute->size() + 1));

        deltaCost
            += costEvaluator.twPenalty(uDS.timeWarp(uRoute->maxDuration()));
    }
    else
    {
        auto const uDS = DurationSegment::merge(data.durationMatrix(),
                                                uRoute->before(U->idx()),
                                                uRoute->at(uRoute->size() + 1));

        deltaCost
            += costEvaluator.twPenalty(uDS.timeWarp(uRoute->maxDuration()));
    }

    if (U->idx() < uRoute->size())
    {
        auto const vDS = DurationSegment::merge(
            data.durationMatrix(),
            vRoute->before(V->idx()),
            uRoute->between(U->idx() + 1, uRoute->size()),
            vRoute->at(vRoute->size() + 1));

        deltaCost
            += costEvaluator.twPenalty(vDS.timeWarp(vRoute->maxDuration()));
    }
    else
    {
        auto const vDS = DurationSegment::merge(data.durationMatrix(),
                                                vRoute->before(V->idx()),
                                                vRoute->at(vRoute->size() + 1));

        deltaCost
            += costEvaluator.twPenalty(vDS.timeWarp(vRoute->maxDuration()));
    }

    auto const uLS = LoadSegment::merge(uRoute->before(U->idx()),
                                        vRoute->after(V->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(uLS.load(), uRoute->capacity());

    auto const vLS = LoadSegment::merge(vRoute->before(V->idx()),
                                        uRoute->after(U->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(vLS.load(), vRoute->capacity());

    return deltaCost;
}

pyvrp::Cost
SwapTails::evaluate(Route *U, Route *V, CostEvaluator const &costEvaluator)
{
    move = {};

    for (size_t uIdx = 0; uIdx != U->size() + 1; ++uIdx)
        for (size_t vIdx = 0; vIdx != V->size() + 1; ++vIdx)
        {
            auto *nodeU = (*U)[uIdx];
            auto *nodeV = (*V)[vIdx];

            auto const deltaCost = evaluate(nodeU, nodeV, costEvaluator);
            if (deltaCost < move.deltaCost)
            {
                move.deltaCost = deltaCost;
                move.U = nodeU;
                move.V = nodeV;
            }
        }

    return move.deltaCost;
}

void SwapTails::apply(Route *U, Route *V) const
{
    auto *nU = n(move.U);
    auto *nV = n(move.V);

    auto insertIdx = move.U->idx() + 1;
    while (!nV->isDepot())
    {
        auto *node = nV;
        nV = n(nV);
        V->remove(node->idx());
        U->insert(insertIdx++, node);
    }

    insertIdx = move.V->idx() + 1;
    while (!nU->isDepot())
    {
        auto *node = nU;
        nU = n(nU);
        U->remove(node->idx());
        V->insert(insertIdx++, node);
    }
}
