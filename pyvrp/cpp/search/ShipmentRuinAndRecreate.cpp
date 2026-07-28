#include "ShipmentRuinAndRecreate.h"

#include "DeliverySegment.h"
#include "PickupSegment.h"

#include <algorithm>
#include <cassert>
#include <limits>

using pyvrp::search::ShipmentRuinAndRecreate;

namespace
{
constexpr double RUIN_PROBABILITY = 0.25;
constexpr size_t MIN_RUIN_SIZE = 8;
constexpr size_t MAX_RUIN_SIZE = 15;
constexpr double LARGE_RUIN_PROBABILITY = 0.05;
constexpr size_t MAX_LARGE_RUIN_SIZE = 30;
constexpr size_t MAX_NUM_RELATED = 100;
constexpr size_t NUM_CANDIDATE_ROUTES = 6;
}  // namespace

ShipmentRuinAndRecreate::ShipmentRuinAndRecreate(ProblemData const &data)
    : data(data), rng_(42)
{
    auto const numShipments = data.numShipments();
    if (numShipments == 0 || data.numClients() != 0)
        return;

    auto const &distances = data.distanceMatrix(0);
    auto const location = [&](size_t activity)
    {
        if (activity < numShipments)
            return data.shipment(activity).pickup.location;

        return data.shipment(activity - numShipments).delivery.location;
    };

    auto const numActivities = 2 * numShipments;
    related_.resize(numActivities);

    std::vector<std::pair<double, size_t>> scored;
    scored.reserve(numActivities);

    for (size_t from = 0; from != numActivities; ++from)
    {
        scored.clear();
        auto const fromLocation = location(from);
        for (size_t to = 0; to != numActivities; ++to)
        {
            if (to == from || (to % numShipments) == (from % numShipments))
                continue;

            auto const toLocation = location(to);
            auto const score
                = static_cast<double>(distances(fromLocation, toLocation))
                  + static_cast<double>(distances(toLocation, fromLocation));
            scored.emplace_back(score, to);
        }

        auto const numRelated = std::min(MAX_NUM_RELATED, scored.size());
        std::partial_sort(
            scored.begin(), scored.begin() + numRelated, scored.end());

        related_[from].reserve(numRelated);
        for (size_t idx = 0; idx != numRelated; ++idx)
            related_[from].push_back(scored[idx].second);
    }
}

bool ShipmentRuinAndRecreate::apply(Solution &solution,
                                    SearchSpace &searchSpace,
                                    CostEvaluator const &costEvaluator) const
{
    if (related_.empty())
        return false;

    if (rng_.rand() >= RUIN_PROBABILITY)
        return false;

    if (!ruin(solution, searchSpace))
        return false;

    recreate(solution, searchSpace, costEvaluator);
    return true;
}

bool ShipmentRuinAndRecreate::ruin(Solution &solution,
                                   SearchSpace &searchSpace) const
{
    auto const numShipments = data.numShipments();

    std::vector<size_t> assignedActivities;
    assignedActivities.reserve(2 * numShipments);
    for (size_t shipment = 0; shipment != numShipments; ++shipment)
        if (solution.shipments[shipment].first.route())
        {
            assignedActivities.push_back(shipment);
            assignedActivities.push_back(numShipments + shipment);
        }

    if (assignedActivities.size() < 2 * MIN_RUIN_SIZE)
        return false;

    auto const seedActivity
        = assignedActivities[rng_.randint(assignedActivities.size())];
    auto const seedShipment = seedActivity % numShipments;
    auto *seedRoute = solution.shipments[seedShipment].first.route();

    std::vector<Route *> routes = {seedRoute};
    auto const numRoutes = 2 + rng_.randint(2);
    for (auto const activity : related_[seedActivity])
    {
        if (routes.size() >= numRoutes)
            break;

        auto *route = solution.shipments[activity % numShipments].first.route();
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

    auto const largeRuin = rng_.rand() < LARGE_RUIN_PROBABILITY;
    size_t targetRuinSize;
    if (largeRuin)
        targetRuinSize = MAX_RUIN_SIZE + 1
                         + rng_.randint(MAX_LARGE_RUIN_SIZE - MAX_RUIN_SIZE);
    else
        targetRuinSize
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
                                       CostEvaluator const &costEvaluator) const
{
    if (related_.empty())
        return;

    auto const numShipments = data.numShipments();

    for (auto const &activity : searchSpace.activityOrder())
    {
        if (!activity.isPickup())
            continue;

        auto const shipment = activity.idx();
        auto *pickup = &solution.shipments[shipment].first;
        if (pickup->route())
            continue;

        auto *delivery = &solution.shipments[shipment].second;

        std::vector<Route *> routes;

        auto const &pickupRelated = related_[shipment];
        auto const &deliveryRelated = related_[numShipments + shipment];
        auto const numRelated
            = std::max(pickupRelated.size(), deliveryRelated.size());

        for (size_t rank = 0; rank != numRelated; ++rank)
        {
            if (routes.size() >= NUM_CANDIDATE_ROUTES)
                break;

            for (auto const *related : {&pickupRelated, &deliveryRelated})
            {
                if (rank >= related->size()
                    || routes.size() >= NUM_CANDIDATE_ROUTES)
                    continue;

                auto const relatedActivity = (*related)[rank];
                auto const relatedShipment = relatedActivity % numShipments;
                auto *route = solution.shipments[relatedShipment].first.route();
                if (!route)
                    continue;

                if (std::find(routes.begin(), routes.end(), route)
                    != routes.end())
                    continue;

                routes.push_back(route);
            }
        }

        for (auto &route : solution.routes)
            if (route.empty())
            {
                routes.push_back(&route);
                break;
            }

        if (routes.empty())
            continue;

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
