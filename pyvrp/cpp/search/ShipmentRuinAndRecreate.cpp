#include "ShipmentRuinAndRecreate.h"

#include "DeliverySegment.h"
#include "PickupSegment.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>

using pyvrp::Activity;
using pyvrp::search::ShipmentRuinAndRecreate;

namespace
{
constexpr double RUIN_PROBABILITY = 0.25;
constexpr size_t MIN_RUIN_SIZE = 8;
constexpr size_t MAX_RUIN_SIZE = 20;
}  // namespace

ShipmentRuinAndRecreate::ShipmentRuinAndRecreate(ProblemData const &data)
    : data(data), rng_(42)
{
}

bool ShipmentRuinAndRecreate::apply(Solution &solution,
                                    SearchSpace &searchSpace,
                                    CostEvaluator const &costEvaluator) const
{
    if (data.numShipments() == 0 || data.numClients() != 0)
        return false;

    if (rng_.rand() >= RUIN_PROBABILITY)
        return false;

    std::vector<Route *> routes;
    if (!ruin(solution, searchSpace, routes))
        return false;

    recreate(solution, searchSpace, routes, costEvaluator);
    return true;
}

bool ShipmentRuinAndRecreate::ruin(Solution &solution,
                                   SearchSpace &searchSpace,
                                   std::vector<Route *> &routes) const
{
    auto const numShipments = data.numShipments();

    std::vector<size_t> assignedShipments;
    assignedShipments.reserve(numShipments);
    for (size_t shipment = 0; shipment != numShipments; ++shipment)
        if (solution.shipments[shipment].first.route())
            assignedShipments.push_back(shipment);

    if (assignedShipments.size() < MIN_RUIN_SIZE)
        return false;

    auto const seedShipment
        = assignedShipments[rng_.randint(assignedShipments.size())];
    auto *seedRoute = solution.shipments[seedShipment].first.route();

    routes = {seedRoute};
    auto const numRoutes = 2 + rng_.randint(2);
    Activity const seedActivity
        = {Activity::ActivityType::PICKUP, seedShipment};
    for (auto const &activity : searchSpace.neighboursOf(seedActivity))
    {
        if (routes.size() >= numRoutes)
            break;

        auto *node = solution[activity];
        assert(node);

        auto *route = node->route();
        if (!route)
            continue;

        if (std::find(routes.begin(), routes.end(), route) != routes.end())
            continue;

        routes.push_back(route);
    }

    if (routes.size() < 2)
        return false;

    size_t numAvailable = 0;
    for (auto const *route : routes)
        numAvailable += route->numShipments();

    auto targetRuinSize
        = MIN_RUIN_SIZE + rng_.randint(MAX_RUIN_SIZE - MIN_RUIN_SIZE + 1);
    targetRuinSize = std::min(targetRuinSize, numAvailable);
    if (targetRuinSize < MIN_RUIN_SIZE)
        return false;

    std::vector<bool> ruined(numShipments, false);
    size_t numRuined = 0;

    for (size_t idx = 0; idx != routes.size() && numRuined < targetRuinSize;
         ++idx)
    {
        auto *route = routes[idx];
        if (route->size() <= 2)
            continue;

        auto const numRoutesLeft = routes.size() - idx;
        auto numToRuin
            = (targetRuinSize - numRuined + numRoutesLeft - 1) / numRoutesLeft;
        numToRuin = std::min(numToRuin, route->numShipments());

        auto const numNodes = route->size() - 2;
        auto position = 1 + rng_.randint(numNodes);
        auto left = position;
        auto right = position;
        size_t numRouteRuined = 0;

        while (numRouteRuined < numToRuin)
        {
            auto *node = (*route)[position];
            if (!node->isDepot())
            {
                auto const shipment = node->idx();
                if (!ruined[shipment])
                {
                    ruined[shipment] = true;
                    ++numRouteRuined;
                    ++numRuined;
                }
            }

            bool const canMoveLeft = left > 1;
            bool const canMoveRight = right < route->size() - 2;
            if (!canMoveLeft && !canMoveRight)
                break;

            if (canMoveRight && (!canMoveLeft || rng_.randint(2)))
                position = ++right;
            else
                position = --left;
        }
    }

    if (numRuined < MIN_RUIN_SIZE)
        return false;

    searchSpace.unmarkAllPromising();

    for (size_t shipment = 0; shipment != numShipments; ++shipment)
    {
        if (!ruined[shipment])
            continue;

        auto *pickup = &solution.shipments[shipment].first;
        auto *delivery = &solution.shipments[shipment].second;
        auto *route = pickup->route();
        assert(route && delivery->route() == route);

        searchSpace.markPromising(pickup);
        searchSpace.markPromising(delivery);

        // Remove in descending position order.
        route->remove(delivery->pos());
        route->remove(pickup->pos());
    }

    for (auto *route : routes)
    {
        route->update();
        if (route->empty())
            route->clear();
    }

    return true;
}

void ShipmentRuinAndRecreate::recreate(Solution &solution,
                                       SearchSpace &searchSpace,
                                       std::vector<Route *> const &routes,
                                       CostEvaluator const &costEvaluator) const
{
    if (data.numShipments() == 0 || data.numClients() != 0)
        return;

    for (auto const &activity : searchSpace.activityOrder())
    {
        if (!activity.isPickup())
            continue;

        auto const shipment = activity.idx();
        auto *pickup = &solution.shipments[shipment].first;
        if (pickup->route())
            continue;

        auto *delivery = &solution.shipments[shipment].second;

        auto const prize = data.shipment(shipment).prize;
        Cost bestCost = std::numeric_limits<Cost>::max();
        Route *bestRoute = nullptr;
        size_t bestPickupPos = 0;
        size_t bestDeliveryPos = 0;

        for (auto *route : routes)
        {
            auto const numPositions = route->size() - 1;
            auto const fixedVehicleCost
                = route->empty() ? route->fixedVehicleCost() : 0;

            for (size_t pickupPos = 0; pickupPos != numPositions; ++pickupPos)
                for (size_t deliveryPos = pickupPos;
                     deliveryPos != numPositions;
                     ++deliveryPos)
                {
                    Cost deltaCost = fixedVehicleCost - prize;
                    if (pickupPos == deliveryPos)
                        costEvaluator.deltaCost<true>(
                            deltaCost,
                            Route::Proposal(route->before(pickupPos),
                                            PickupSegment(data, shipment),
                                            DeliverySegment(data, shipment),
                                            route->after(pickupPos + 1)));
                    else
                        costEvaluator.deltaCost<true>(
                            deltaCost,
                            Route::Proposal(
                                route->before(pickupPos),
                                PickupSegment(data, shipment),
                                route->between(pickupPos + 1, deliveryPos),
                                DeliverySegment(data, shipment),
                                route->after(deliveryPos + 1)));

                    if (deltaCost < bestCost)
                    {
                        bestCost = deltaCost;
                        bestRoute = route;
                        bestPickupPos = pickupPos;
                        bestDeliveryPos = deliveryPos;
                    }
                }
        }

        if (!bestRoute)
            continue;

        bestRoute->insert(bestDeliveryPos + 1, delivery);
        bestRoute->insert(bestPickupPos + 1, pickup);
        bestRoute->update();

        assert(pickup->route() == delivery->route());
        assert(pickup->pos() < delivery->pos());

        searchSpace.markPromising(pickup);
        searchSpace.markPromising(delivery);
    }
}
