#include "MoveTwoClientsReversed.h"
#include "Route.h"
#include "TimeWindowSegment.h"

#include <cassert>

using TWS = TimeWindowSegment;

Cost MoveTwoClientsReversed::evaluate(Node *U,
                                      Node *V,
                                      CostEvaluator const &costEvaluator)
{
    if (U == n(V) || n(U) == V || n(U)->isDepot())
        return 0;

    if(checkSalvageSequenceConstraint(U, V)) {
        return std::numeric_limits<Cost>::max() / 1000;
    }

    auto const posU = U->position;
    auto const posV = V->position;

    assert(U->route && V->route);

    Distance const current = U->route->distBetween(posU - 1, posU + 2)
                             + data.dist(V->client, n(V)->client);
    Distance const proposed = data.dist(p(U)->client, n(n(U))->client)
                              + data.dist(V->client, n(U)->client)
                              + data.dist(n(U)->client, U->client)
                              + data.dist(U->client, n(V)->client);

    Cost deltaCost = static_cast<Cost>(proposed - current);

    if (U->route != V->route)
    {
        if (U->route->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uTWS = TWS::merge(
            data.durationMatrix(), p(U)->twBefore, n(n(U))->twAfter);

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());

        auto const weightDiff = U->route->weightBetween(posU, posU + 1);
        auto const volumeDiff = U->route->volumeBetween(posU, posU + 1);
        auto const salvageDiff = U->route->salvageBetween(posU, posU + 1);

        deltaCost += costEvaluator.weightPenalty(U->route->weight() - weightDiff,
                                               data.weightCapacity());
        deltaCost += costEvaluator.volumePenalty(U->route->volume() - volumeDiff,
                                               data.volumeCapacity());
        deltaCost += costEvaluator.salvagePenalty(U->route->salvage() - salvageDiff,
                                               data.salvageCapacity());
        deltaCost -= costEvaluator.weightPenalty(U->route->weight(),
                                               data.weightCapacity());
        deltaCost -= costEvaluator.volumePenalty(U->route->volume(),
                                               data.volumeCapacity());
        deltaCost -= costEvaluator.salvagePenalty(U->route->salvage(),
                                               data.salvageCapacity());

        if (deltaCost >= 0)    // if delta cost of just U's route is not enough
            return deltaCost;  // even without V, the move will never be good

        deltaCost += costEvaluator.weightPenalty(V->route->weight() + weightDiff,
                                               data.weightCapacity());
        deltaCost += costEvaluator.volumePenalty(V->route->volume() + volumeDiff,
                                               data.volumeCapacity());
        deltaCost += costEvaluator.salvagePenalty(V->route->salvage() + salvageDiff,
                                               data.salvageCapacity());
        deltaCost -= costEvaluator.weightPenalty(V->route->weight(),
                                               data.weightCapacity());
        deltaCost -= costEvaluator.volumePenalty(V->route->volume(),
                                               data.volumeCapacity());
        deltaCost -= costEvaluator.salvagePenalty(V->route->salvage(),
                                               data.salvageCapacity());

        auto vTWS = TWS::merge(
            data.durationMatrix(), V->twBefore, n(U)->tw, U->tw, n(V)->twAfter);

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(V->route->timeWarp());
    }
    else  // within same route
    {
        auto const *route = U->route;

        if (!route->hasTimeWarp() && deltaCost >= 0)
            return deltaCost;

        if (posU < posV)
        {
            auto const uTWS = TWS::merge(data.durationMatrix(),
                                         p(U)->twBefore,
                                         route->twBetween(posU + 2, posV),
                                         n(U)->tw,
                                         U->tw,
                                         n(V)->twAfter);

            deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        }
        else
        {
            auto const uTWS = TWS::merge(data.durationMatrix(),
                                         V->twBefore,
                                         n(U)->tw,
                                         U->tw,
                                         route->twBetween(posV + 1, posU - 1),
                                         n(n(U))->twAfter);

            deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        }

        deltaCost -= costEvaluator.twPenalty(route->timeWarp());
    }

    return deltaCost;
}

bool MoveTwoClientsReversed::checkSalvageSequenceConstraint(Node *U, Node *V) const
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

void MoveTwoClientsReversed::apply(Node *U, Node *V) const
{
    auto *X = n(U);  // copy since the insert below changes n(U)

    U->insertAfter(V);
    X->insertAfter(V);
}
