#include "PerturbationManager.h"

#include "ClientSegment.h"
#include "DeliverySegment.h"
#include "DynamicBitset.h"
#include "PickupSegment.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <vector>

using pyvrp::Activity;
using pyvrp::search::PerturbationManager;
using pyvrp::search::PerturbationParams;
using pyvrp::search::Route;

namespace
{
enum class PerturbType
{
    REMOVE,
    INSERT
};
}  // namespace

PerturbationParams::PerturbationParams(size_t minPerturbations,
                                       size_t maxPerturbations)
    : minPerturbations(minPerturbations), maxPerturbations(maxPerturbations)
{
    if (minPerturbations > maxPerturbations)
        throw std::invalid_argument(
            "min_perturbations must be <= max_perturbations.");
}

PerturbationManager::PerturbationManager(PerturbationParams params)
    : params_(params),
      numPerturbations_(params_.minPerturbations),
      numRoutes_(1)
{
}

size_t PerturbationManager::numPerturbations() const
{
    return numPerturbations_;
}

void PerturbationManager::shuffle(RandomNumberGenerator &rng)
{
    auto const range = params_.maxPerturbations - params_.minPerturbations;
    numPerturbations_ = params_.minPerturbations + rng.randint(range + 1);
    numRoutes_ = 1 + rng.randint(3);  // TODO
}

PerturbationManager::RuinResult
PerturbationManager::ruin(Solution &solution, SearchSpace &searchSpace) const
{
    Route::Node *seed = nullptr;
    for (auto const &activity : searchSpace.activityOrder())
    {
        auto *node = solution[activity];
        assert(node);

        if (node->route())
        {
            seed = node;
            break;
        }
    }

    if (!seed)
        return {};

    std::vector<Route *> routes = {seed->route()};
    std::vector<Activity> activities;
    std::vector<size_t> positions = {seed->pos()};
    auto const numRoutes = std::min(numRoutes_, numPerturbations_);
    for (auto const &activity : searchSpace.neighboursOf(seed->activity()))
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
        positions.push_back(node->pos());
    }

    size_t numAvailable = 0;
    for (auto const *route : routes)
        numAvailable += route->numClients() + route->numShipments();

    auto const targetRuinSize = std::min(numPerturbations_, numAvailable);

    auto const numClients = solution.clients.size();
    DynamicBitset selected(numClients + solution.shipments.size());
    for (size_t idx = 0; idx != routes.size(); ++idx)
    {
        auto *route = routes[idx];

        // Divide the remaining ruin size over the remaining routes, rounding
        // up so the full target can still be reached.
        auto const numRoutesLeft = routes.size() - idx;
        auto const numRemaining = targetRuinSize - activities.size();
        auto const targetForRoute
            = (numRemaining + numRoutesLeft - 1) / numRoutesLeft;
        auto movesLeft = std::min(targetForRoute,
                                  route->numClients() + route->numShipments());

        // Walk forward from the selected position, wrapping around the route.
        auto const numNodes = route->size() - 2;
        auto position = positions[idx];
        for (size_t offset = 0; offset != numNodes && movesLeft != 0; ++offset)
        {
            auto *node = (*route)[position];
            position = position == numNodes ? 1 : position + 1;

            if (!node->isClient() && !node->isShipment())
                continue;

            auto const selectedIdx
                = node->isClient() ? node->idx() : numClients + node->idx();
            if (selected[selectedIdx])
                continue;

            selected[selectedIdx] = true;
            // Store shipments by pickup, even when a delivery was selected.
            activities.push_back(
                node->isClient()
                    ? node->activity()
                    : Activity(Activity::ActivityType::PICKUP, node->idx()));
            movesLeft--;
        }
    }

    searchSpace.unmarkAllPromising();

    // Select all requests before mutating the routes traversed above.
    for (auto const &activity : activities)
    {
        auto *node = solution[activity];
        assert(node && node->route());

        auto *route = node->route();
        searchSpace.markPromising(node);
        if (node->isClient())
        {
            route->remove(node->pos());
            continue;
        }

        assert(node->isPickup());
        auto *delivery = node + 1;
        assert(delivery->route() == route);
        searchSpace.markPromising(delivery);
        route->remove(delivery->pos());
        route->remove(node->pos());
    }

    for (auto *route : routes)
    {
        route->update();
        if (route->empty())
            route->clear();
    }

    return {std::move(routes), std::move(activities)};
}

void PerturbationManager::recreate(Solution &solution,
                                   SearchSpace &searchSpace,
                                   RuinResult const &result,
                                   CostEvaluator const &costEvaluator) const
{
    auto const &data = solution.data_;

    // Shuffle activities in random order.
    auto const numClients = solution.clients.size();
    DynamicBitset selected(numClients + solution.shipments.size());
    for (auto const &activity : result.activities)
    {
        auto const selectedIdx = activity.isClient()
                                     ? activity.idx()
                                     : numClients + activity.idx();
        selected[selectedIdx] = true;
    }

    std::vector<Activity> activities;
    activities.reserve(result.activities.size());
    for (auto const &activity : searchSpace.activityOrder())
    {
        auto const selectedIdx = activity.isClient()
                                     ? activity.idx()
                                     : numClients + activity.idx();
        if (selected[selectedIdx])
            activities.push_back(activity);
    }

    for (auto const &activity : activities)
    {
        auto *node = solution[activity];
        assert(node && !node->route());

        if (node->isClient())
        {
            auto const &client = data.client(node->idx());
            auto const required
                = client.required
                  || (client.group && data.group(*client.group).required);

            Cost bestCost = std::numeric_limits<Cost>::max();
            Route *bestRoute = nullptr;
            size_t bestPosition = 0;
            for (auto *route : result.routes)
            {
                auto const numPositions = route->size() - 1;
                auto const fixedVehicleCost
                    = route->empty() ? route->fixedVehicleCost() : 0;
                for (size_t position = 0; position != numPositions; ++position)
                {
                    Cost deltaCost = fixedVehicleCost - client.prize;
                    costEvaluator.deltaCost<true>(
                        deltaCost,
                        Route::Proposal(route->before(position),
                                        ClientSegment(data, node->idx()),
                                        route->after(position + 1)));

                    if (deltaCost < bestCost)
                    {
                        bestCost = deltaCost;
                        bestRoute = route;
                        bestPosition = position;
                    }
                }
            }

            assert(bestRoute);
            if (!required && bestCost >= 0)
                continue;

            bestRoute->insert(bestPosition + 1, node);
            bestRoute->update();
            searchSpace.markPromising(node);
            continue;
        }

        assert(node->isPickup());
        auto *pickup = node;
        auto *delivery = pickup + 1;
        assert(!delivery->route());

        auto const &shipment = data.shipment(pickup->idx());
        Cost bestCost = std::numeric_limits<Cost>::max();
        Route *bestRoute = nullptr;
        size_t bestPickupPos = 0;
        size_t bestDeliveryPos = 0;
        for (auto *route : result.routes)
        {
            auto const numPositions = route->size() - 1;
            auto const fixedVehicleCost
                = route->empty() ? route->fixedVehicleCost() : 0;
            for (size_t pickupPos = 0; pickupPos != numPositions; ++pickupPos)
                for (size_t deliveryPos = pickupPos;
                     deliveryPos != numPositions;
                     ++deliveryPos)
                {
                    Cost deltaCost = fixedVehicleCost - shipment.prize;
                    if (pickupPos == deliveryPos)
                        costEvaluator.deltaCost<true>(
                            deltaCost,
                            Route::Proposal(
                                route->before(pickupPos),
                                PickupSegment(data, pickup->idx()),
                                DeliverySegment(data, pickup->idx()),
                                route->after(pickupPos + 1)));
                    else
                        costEvaluator.deltaCost<true>(
                            deltaCost,
                            Route::Proposal(
                                route->before(pickupPos),
                                PickupSegment(data, pickup->idx()),
                                route->between(pickupPos + 1, deliveryPos),
                                DeliverySegment(data, pickup->idx()),
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

        assert(bestRoute);
        if (!shipment.required && bestCost >= 0)
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

void PerturbationManager::perturb(Solution &solution,
                                  SearchSpace &searchSpace,
                                  CostEvaluator const &costEvaluator) const
{
    size_t movesLeft = numPerturbations_;

    if (!movesLeft)  // nothing to do
        return;

    auto const result = ruin(solution, searchSpace);
    if (!result.routes.empty())
    {
        recreate(solution, searchSpace, result, costEvaluator);
        return;
    }

    // Clear the set of promising nodes. Perturbation determines the initial
    // set of promising nodes for further (local search) improvement.
    searchSpace.unmarkAllPromising();

    DynamicBitset perturbed
        = {solution.clients.size() + solution.shipments.size()};
    auto const perturb = [&](Route::Node *node, PerturbType action)
    {
        assert(node->isClient() || node->isPickup());
        auto const idx = node->isClient()
                             ? node->idx()
                             : solution.clients.size() + node->idx();

        // This node has already been touched by a previous perturbation, so
        // we skip it here.
        if (perturbed[idx])
            return;

        // Remove if node is in a route and we are currently removing.
        auto *route = node->route();
        if (route && action == PerturbType::REMOVE)
        {
            searchSpace.markPromising(node);
            route->remove(node->pos());

            if (node->isPickup())  // then we also remove the associated
            {                      // delivery node
                auto const *delivery = node + 1;
                assert(delivery->route() == route);

                searchSpace.markPromising(delivery);
                route->remove(delivery->pos());
            }

            route->update();
        }
        // Insert if node is not in a route and we are currently inserting.
        else if (!route && action == PerturbType::INSERT)
        {
            if (node->isClient())
            {
                solution.insert(node, searchSpace, costEvaluator, true);
                node->route()->update();
                searchSpace.markPromising(node);
            }
            else if (node->isPickup())
            {
                auto *pickup = node;
                auto *delivery = node + 1;
                solution.insert(
                    pickup, delivery, searchSpace, costEvaluator, true);

                auto *route = pickup->route();
                assert(delivery->route() == route);

                route->update();
                searchSpace.markPromising(pickup);
                searchSpace.markPromising(delivery);
            }
        }
        else  // no-op
            return;

        perturbed[idx] = true;
        movesLeft--;
    };

    // We do numPerturbations if we can. We perturb the local neighbourhood of
    // a randomly selected clients or pickups: if a selected client or pickup U
    // is in the solution, we remove it and its neighbours V. If it is not, we
    // try to insert instead. Each removal or insertion counts as one
    // perturbation.
    for (auto const &uActivity : searchSpace.activityOrder())
    {
        Route::Node *U = solution[uActivity];
        assert(U);

        auto action = U->route() ? PerturbType::REMOVE : PerturbType::INSERT;
        perturb(U, action);

        if (!movesLeft)
            return;

        for (auto const &vActivity : searchSpace.neighboursOf(uActivity))
        {
            Route::Node *V = nullptr;
            switch (vActivity.type())
            {
            case Activity::ActivityType::CLIENT:
                V = &solution.clients[vActivity.idx()];
                break;

            case Activity::ActivityType::PICKUP:  // with shipments we perturb
                [[fallthrough]];                  // the pickup, not delivery
            case Activity::ActivityType::DELIVERY:
                V = &solution.shipments[vActivity.idx()].first;
                break;

            default:
                continue;
            }

            assert(V);
            perturb(V, action);

            if (!movesLeft)
                return;
        }
    }
}
