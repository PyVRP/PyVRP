#include "TwoOpt.h"

#include "LoadSegment.h"
#include "Route.h"
#include "TimeWindowSegment.h"

#include <cassert>

using pyvrp::Cost;
using pyvrp::search::TwoOpt;
using TWS = pyvrp::TimeWindowSegment;

Cost TwoOpt::evalWithinRoute(Route::Node *U,
                             Route::Node *V,
                             CostEvaluator const &costEvaluator) const
{
    assert(U->route() == V->route());
    auto *route = U->route();

    // Current situation is U -> n(U) -> ... -> V -> n(V). Proposed move is
    // U -> V -> p(V) -> ... -> n(U) -> n(V). This reverses the segment from
    // n(U) to V.
    Distance segmentReversalDistance = 0;  // reversal dist of n(U) -> ... -> V
    for (auto *node = V; node != n(U); node = p(node))
        segmentReversalDistance += data.dist(node->client(), p(node)->client());

    auto const deltaDist = data.dist(U->client(), V->client())
                           + data.dist(n(U)->client(), n(V)->client())
                           + segmentReversalDistance
                           - data.dist(U->client(), n(U)->client())
                           - data.dist(V->client(), n(V)->client())
                           - Distance(route->between(U->idx() + 1, V->idx()));

    Cost deltaCost
        = static_cast<Cost>(deltaDist)
          - costEvaluator.loadPenalty(route->load(), route->capacity())
          - costEvaluator.twPenalty(route->timeWarp());

    if (deltaCost >= 0)
        return deltaCost;

    LoadSegment ls = route->before(U->idx());
    for (size_t idx = V->idx(); idx != U->idx(); --idx)
        ls = LoadSegment::merge(ls, route->at(idx));
    ls = LoadSegment::merge(ls, route->after(V->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(ls.load(), route->capacity());

    TimeWindowSegment tws = route->before(U->idx());
    for (size_t idx = V->idx(); idx != U->idx(); --idx)
        tws = TWS::merge(data.durationMatrix(), tws, route->at(idx));
    tws = TWS::merge(data.durationMatrix(), tws, route->after(V->idx() + 1));

    deltaCost += costEvaluator.twPenalty(tws.timeWarp(route->maxDuration()));

    return deltaCost;
}

Cost TwoOpt::evalBetweenRoutes(Route::Node *U,
                               Route::Node *V,
                               CostEvaluator const &costEvaluator) const
{
    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    // Two routes. Current situation is U -> n(U), and V -> n(V). Proposed move
    // is U -> n(V) and V -> n(U).
    Distance current = data.dist(U->client(), n(U)->client())
                       + data.dist(V->client(), n(V)->client());

    // Proposed distances are either to the other segment, if that segment
    // exists, or back to the depot.  Some caveats are handled below.
    auto const nU = n(V)->isDepot() ? uRoute->depot() : n(V)->client();
    auto const nV = n(U)->isDepot() ? vRoute->depot() : n(U)->client();
    Distance proposed = data.dist(U->client(), nU) + data.dist(V->client(), nV);

    // If n(U) is not the end depot, we might have distance changes due to the
    // segment after U now ending at V's route's depot.
    if (!n(U)->isDepot())
    {
        auto const *endU = p(*uRoute->end());
        proposed += data.dist(endU->client(), vRoute->depot());
        current += data.dist(endU->client(), uRoute->depot());
    }

    // If n(V) is not the end depot, we might have distance changes due to the
    // segment after V now ending at U's route's depot.
    if (!n(V)->isDepot())
    {
        auto const *endV = p(*vRoute->end());
        proposed += data.dist(endV->client(), uRoute->depot());
        current += data.dist(endV->client(), vRoute->depot());
    }

    Cost deltaCost = static_cast<Cost>(proposed - current);

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

    if (uRoute->isFeasible() && vRoute->isFeasible() && deltaCost >= 0)
        return deltaCost;

    if (V->idx() < vRoute->size())
    {
        auto const uTWS
            = TWS::merge(data.durationMatrix(),
                         uRoute->before(U->idx()),
                         vRoute->between(V->idx() + 1, vRoute->size()),
                         uRoute->at(uRoute->size() + 1));

        deltaCost
            += costEvaluator.twPenalty(uTWS.timeWarp(uRoute->maxDuration()));
    }
    else
    {
        auto const uTWS = TWS::merge(data.durationMatrix(),
                                     uRoute->before(U->idx()),
                                     uRoute->at(uRoute->size() + 1));

        deltaCost
            += costEvaluator.twPenalty(uTWS.timeWarp(uRoute->maxDuration()));
    }

    if (U->idx() < uRoute->size())
    {
        auto const vTWS
            = TWS::merge(data.durationMatrix(),
                         vRoute->before(V->idx()),
                         uRoute->between(U->idx() + 1, uRoute->size()),
                         vRoute->at(vRoute->size() + 1));

        deltaCost
            += costEvaluator.twPenalty(vTWS.timeWarp(vRoute->maxDuration()));
    }
    else
    {
        auto const vTWS = TWS::merge(data.durationMatrix(),
                                     vRoute->before(V->idx()),
                                     vRoute->at(vRoute->size() + 1));

        deltaCost
            += costEvaluator.twPenalty(vTWS.timeWarp(vRoute->maxDuration()));
    }

    deltaCost -= costEvaluator.twPenalty(uRoute->timeWarp());
    deltaCost -= costEvaluator.twPenalty(vRoute->timeWarp());

    auto const uLS = LoadSegment::merge(uRoute->before(U->idx()),
                                        vRoute->after(V->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(uLS.load(), uRoute->capacity());
    deltaCost -= costEvaluator.loadPenalty(uRoute->load(), uRoute->capacity());

    auto const vLS = LoadSegment::merge(vRoute->before(V->idx()),
                                        uRoute->after(U->idx() + 1));

    deltaCost += costEvaluator.loadPenalty(vLS.load(), vRoute->capacity());
    deltaCost -= costEvaluator.loadPenalty(vRoute->load(), vRoute->capacity());

    return deltaCost;
}

void TwoOpt::applyWithinRoute(Route::Node *U, Route::Node *V) const
{
    auto *nU = n(U);

    while (V->idx() > nU->idx())
    {
        auto *pV = p(V);
        Route::swap(nU, V);
        nU = n(V);  // after swap, V is now nU
        V = pV;
    }
}

void TwoOpt::applyBetweenRoutes(Route::Node *U, Route::Node *V) const
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

Cost TwoOpt::evaluate(Route::Node *U,
                      Route::Node *V,
                      CostEvaluator const &costEvaluator)
{
    assert(U->route());
    assert(V->route());

    if (U->route()->idx() > V->route()->idx())  // tackled in a later iteration
        return 0;

    if (U->route() != V->route())
        return evalBetweenRoutes(U, V, costEvaluator);

    if (U->idx() + 1 >= V->idx())  // tackled in a later iteration
        return 0;

    return evalWithinRoute(U, V, costEvaluator);
}

void TwoOpt::apply(Route::Node *U, Route::Node *V) const
{
    if (U->route() == V->route())
        applyWithinRoute(U, V);
    else
        applyBetweenRoutes(U, V);
}
