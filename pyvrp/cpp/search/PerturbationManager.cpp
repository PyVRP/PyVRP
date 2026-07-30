#include "PerturbationManager.h"

#include "DynamicBitset.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>

using pyvrp::search::PerturbationManager;
using pyvrp::search::PerturbationParams;
using pyvrp::search::Route;

namespace
{
enum class PerturbType
{
    REMOVE,
    INSERT,
    ROUTE_REMOVAL,
};
}

PerturbationParams::PerturbationParams(size_t minPerturbations,
                                       size_t maxPerturbations)
    : minPerturbations(minPerturbations), maxPerturbations(maxPerturbations)
{
    if (minPerturbations > maxPerturbations)
        throw std::invalid_argument(
            "min_perturbations must be <= max_perturbations.");
}

PerturbationManager::PerturbationManager(PerturbationParams params)
    : params_(params), numPerturbations_(params_.minPerturbations)
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
    routeRemoval_ = rng.randint(2) == 1;
}

void PerturbationManager::perturb(Solution &solution,
                                  SearchSpace &searchSpace,
                                  CostEvaluator const &costEvaluator) const
{
    size_t movesLeft = numPerturbations_;

    if (!movesLeft)  // nothing to do
        return;

    // Clear the set of promising nodes. Perturbation determines the initial
    // set of promising nodes for further (local search) improvement.
    searchSpace.unmarkAllPromising();

    auto const numClients = solution.clients.size();
    auto const numActivities = numClients + solution.shipments.size();
    DynamicBitset perturbed = {numActivities};
    DynamicBitset removedShipments = {solution.shipments.size()};
    std::vector<Route *> affectedRoutes;

    auto const nodeFor = [&](Activity const &activity) -> Route::Node *
    {
        if (activity.isClient())
            return &solution.clients[activity.idx()];

        if (activity.isShipment())
            return &solution.shipments[activity.idx()].first;

        return nullptr;
    };

    auto const perturbationIdx = [&](Route::Node const *node)
    {
        assert(node->isClient() || node->isPickup());
        return node->isClient() ? node->idx() : numClients + node->idx();
    };

    auto const perturb
        = [&](Route::Node *node, PerturbType action, bool updateRoute = true)
    {
        if (!movesLeft)
            return;

        assert(node->isClient() || node->isPickup());
        auto const idx = perturbationIdx(node);

        // This node has already been touched by a previous perturbation, so
        // we skip it here.
        if (perturbed[idx])
            return;

        // Remove if node is in a route and we are currently removing.
        auto *route = node->route();
        if (route && action == PerturbType::REMOVE)
        {
            if (std::find(affectedRoutes.begin(), affectedRoutes.end(), route)
                == affectedRoutes.end())
                affectedRoutes.push_back(route);

            searchSpace.markPromising(node);

            if (node->isPickup())  // then we also remove the associated
            {                      // delivery node
                auto *delivery = node + 1;
                assert(delivery->route() == route);

                searchSpace.markPromising(delivery);
                route->remove(delivery->pos());
            }

            route->remove(node->pos());
            if (updateRoute)
            {
                route->update();

                if (route->empty())
                    route->clear();
            }

            if (node->isPickup())
                removedShipments[node->idx()] = true;
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

    auto const removeFromRoute = [&](Route::Node *node)
    {
        if (perturbed[perturbationIdx(node)])
            return;

        auto *route = node->route();
        assert(route);

        auto const numPositions = route->size() - 2;
        auto const startPos = node->pos();
        std::vector<Route::Node *> nodes;

        for (size_t offset = 0;
             offset != numPositions && nodes.size() < movesLeft;
             ++offset)
        {
            auto const pos = 1 + (startPos - 1 + offset) % numPositions;
            auto *candidate = (*route)[pos];

            if (candidate->isDepot())
                continue;

            if (candidate->isDelivery())
                candidate = &solution.shipments[candidate->idx()].first;

            auto const idx = perturbationIdx(candidate);
            auto const selected
                = std::find(nodes.begin(), nodes.end(), candidate)
                  != nodes.end();
            if (perturbed[idx] || selected)
                continue;

            nodes.push_back(candidate);
        }

        if (nodes.empty())
            return;

        for (auto *candidate : nodes)
            perturb(candidate, PerturbType::REMOVE, false);

        route->update();
        if (route->empty())
            route->clear();
    };

    // We do numPerturbations if we can. Planned seeds trigger either
    // neighbourhood or route removal, while unplanned seeds trigger
    // neighbourhood insertion. Each client or complete shipment counts as one
    // perturbation.
    auto const removalType
        = routeRemoval_ ? PerturbType::ROUTE_REMOVAL : PerturbType::REMOVE;
    for (auto const &uActivity : searchSpace.activityOrder())
    {
        if (!movesLeft)
            break;

        auto *U = nodeFor(uActivity);
        assert(U);

        auto const action = U->route() ? removalType : PerturbType::INSERT;
        if (action == PerturbType::ROUTE_REMOVAL)
        {
            removeFromRoute(U);
            continue;
        }

        perturb(U, action);

        for (auto const &vActivity : searchSpace.neighboursOf(uActivity))
        {
            if (!movesLeft)
                break;

            auto *V = nodeFor(vActivity);
            if (!V)
                continue;

            perturb(V, action);
        }
    }

    // Recreate only required shipments, in the shuffled activity order.
    for (auto const &activity : searchSpace.activityOrder())
    {
        if (!activity.isPickup() || !removedShipments[activity.idx()])
            continue;

        auto &[pickup, delivery] = solution.shipments[activity.idx()];
        if (!solution.insert(&pickup, affectedRoutes, costEvaluator))
            continue;

        auto *route = pickup.route();
        assert(route);
        route->update();
        assert(delivery.route() == route);
        assert(pickup.trip() == delivery.trip());
        searchSpace.markPromising(&pickup);
        searchSpace.markPromising(&delivery);
    }
}
