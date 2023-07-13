#include "TwoOpt.h"

#include "Route.h"
#include "TimeWindowSegment.h"

using TWS = TimeWindowSegment;

Cost TwoOpt::evalWithinRoute(Node *U,
                             Node *V,
                             CostEvaluator const &costEvaluator) const
{

    if (checkSalvageSequenceConstraint(U, V)) {
        return std::numeric_limits<Cost>::max() / 1000;
    }

    if (U->position + 1 >= V->position)
        return 0;

    Distance const deltaDist = data.dist(U->client, V->client)
                               + data.dist(n(U)->client, n(V)->client)
                               + V->cumulatedReversalDistance
                               - data.dist(U->client, n(U)->client)
                               - data.dist(V->client, n(V)->client)
                               - n(U)->cumulatedReversalDistance;

    Cost deltaCost = static_cast<Cost>(deltaDist);

    if (!U->route->hasTimeWarp() && deltaCost >= 0)
        return deltaCost;

    auto tws = U->twBefore;
    auto *itRoute = V;
    while (itRoute != U)
    {
        tws = TWS::merge(data.durationMatrix(), tws, itRoute->tw);
        itRoute = p(itRoute);
    }

    tws = TWS::merge(data.durationMatrix(), tws, n(V)->twAfter);

    deltaCost += costEvaluator.twPenalty(tws.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());

    return deltaCost;
}

Cost TwoOpt::evalBetweenRoutes(Node *U,
                               Node *V,
                               CostEvaluator const &costEvaluator) const
{

    if (checkSalvageSequenceConstraint(U, V)) {
        return std::numeric_limits<Cost>::max() / 1000;
    }

    Distance const current = data.dist(U->client, n(U)->client)
                            + data.dist(V->client, n(V)->client);
    Distance const proposed = data.dist(U->client, n(V)->client)
                              + data.dist(V->client, n(U)->client);

    Cost deltaCost = static_cast<Cost>(proposed - current);

    if (U->route->isFeasible() && V->route->isFeasible() && deltaCost >= 0)
        return deltaCost;

    auto const uTWS
        = TWS::merge(data.durationMatrix(), U->twBefore, n(V)->twAfter);

    deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());

    auto const vTWS
        = TWS::merge(data.durationMatrix(), V->twBefore, n(U)->twAfter);

    deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(V->route->timeWarp());

    auto const deltaWeight = U->cumulatedWeight - V->cumulatedWeight;
    auto const deltaVolume = U->cumulatedVolume - V->cumulatedVolume;
    auto const deltaSalvage = U->cumulatedSalvage - V->cumulatedSalvage;

    deltaCost += costEvaluator.weightPenalty(U->route->weight() - deltaWeight,
                                           data.weightCapacity());
    deltaCost += costEvaluator.volumePenalty(U->route->volume() - deltaVolume,
                                           data.volumeCapacity());
    deltaCost += costEvaluator.salvagePenalty(U->route->salvage() - deltaSalvage,
                                           data.salvageCapacity());

    deltaCost
        -= costEvaluator.weightPenalty(U->route->weight(), data.weightCapacity());
    deltaCost
        -= costEvaluator.volumePenalty(U->route->volume(), data.volumeCapacity());
    deltaCost
        -= costEvaluator.salvagePenalty(U->route->salvage(), data.salvageCapacity());

    deltaCost += costEvaluator.weightPenalty(V->route->weight() + deltaWeight,
                                           data.weightCapacity());
    deltaCost += costEvaluator.volumePenalty(V->route->volume() + deltaVolume,
                                           data.volumeCapacity());
    deltaCost += costEvaluator.salvagePenalty(V->route->salvage() + deltaSalvage,
                                           data.salvageCapacity());

    deltaCost
        -= costEvaluator.weightPenalty(V->route->weight(), data.weightCapacity());
    deltaCost
        -= costEvaluator.volumePenalty(V->route->volume(), data.volumeCapacity());
    deltaCost
        -= costEvaluator.salvagePenalty(V->route->salvage(), data.salvageCapacity());

    return deltaCost;
}

bool TwoOpt::checkSalvageSequenceConstraint(Node *U, Node *V) const
{
    // These sequences should violate the constraint
    // S-B
    // S-D
    // B-B
    // B-D
    bool uIsClientDelivery = (data.client(U->client).demandWeight || data.client(U->client).demandVolume);
    bool uIsClientSalvage = (data.client(U->client).demandSalvage != Measure<MeasureType::SALVAGE>(0));
    bool uIsBoth = uIsClientDelivery && uIsClientSalvage;

    bool vIsClientDelivery = (data.client(V->client).demandWeight || data.client(V->client).demandVolume);
    bool vIsClientSalvage = (data.client(V->client).demandSalvage != Measure<MeasureType::SALVAGE>(0));
    bool vIsBoth = vIsClientDelivery && vIsClientSalvage;

    bool nextUClientDelivery = (data.client(n(U)->client).demandWeight || data.client(n(U)->client).demandVolume);
    bool nextVClientDelivery = (data.client(n(V)->client).demandWeight || data.client(n(V)->client).demandVolume);

    // S-B or S-D
    if (uIsClientSalvage && !uIsBoth && ((vIsClientDelivery || vIsBoth) || nextVClientDelivery))
        return true;

    // B-B or B-D
    if (uIsBoth && ((vIsBoth || vIsClientDelivery) || nextUClientDelivery))
        return true;

    return false;
}

void TwoOpt::applyWithinRoute(Node *U, Node *V) const
{
    auto *itRoute = V;
    auto *insertionPoint = U;
    auto *currNext = n(U);

    while (itRoute != currNext)  // No need to move x, we pivot around it
    {
        auto *current = itRoute;
        itRoute = p(itRoute);
        current->insertAfter(insertionPoint);
        insertionPoint = current;
    }
}

void TwoOpt::applyBetweenRoutes(Node *U, Node *V) const
{
    auto *itRouteU = n(U);
    auto *itRouteV = n(V);

    auto *insertLocation = U;
    while (!itRouteV->isDepot())
    {
        auto *node = itRouteV;
        itRouteV = n(itRouteV);
        node->insertAfter(insertLocation);
        insertLocation = node;
    }

    insertLocation = V;
    while (!itRouteU->isDepot())
    {
        auto *node = itRouteU;
        itRouteU = n(itRouteU);
        node->insertAfter(insertLocation);
        insertLocation = node;
    }
}

Cost TwoOpt::evaluate(Node *U, Node *V, CostEvaluator const &costEvaluator)
{
    if (U->route->idx > V->route->idx)  // will be tackled in a later iteration
        return 0;                       // - no need to process here already

    return U->route == V->route ? evalWithinRoute(U, V, costEvaluator)
                                : evalBetweenRoutes(U, V, costEvaluator);
}

void TwoOpt::apply(Node *U, Node *V) const
{
    if (U->route == V->route)
        applyWithinRoute(U, V);
    else
        applyBetweenRoutes(U, V);
}
