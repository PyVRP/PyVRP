#include "PerturbationManager.h"

#include "DynamicBitset.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>

using pyvrp::Activity;
using pyvrp::search::PerturbationManager;
using pyvrp::search::PerturbationParams;
using pyvrp::search::Route;
using pyvrp::search::Solution;

namespace
{
enum class PerturbType
{
    REMOVE,
    INSERT
};
}

PerturbationParams::PerturbationParams(size_t minPerturbations,
                                       size_t maxPerturbations,
                                       size_t minRoutes,
                                       size_t maxRoutes)
    : minPerturbations(minPerturbations),
      maxPerturbations(maxPerturbations),
      minRoutes(minRoutes),
      maxRoutes(maxRoutes)
{
    if (minPerturbations > maxPerturbations)
        throw std::invalid_argument(
            "min_perturbations must be <= max_perturbations.");

    if (minRoutes > maxRoutes)
        throw std::invalid_argument("min_routes must be <= max_routes.");
}

PerturbationManager::PerturbationManager(PerturbationParams params)
    : params_(params),
      numPerturbations_(params_.minPerturbations),
      numRoutes_(params_.minRoutes)
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
    useRoutePerturb_ = rng.randint(2) == 1;

    auto const routeRange = params_.maxRoutes - params_.minRoutes;
    numRoutes_ = params_.minRoutes + rng.randint(routeRange + 1);
}

void PerturbationManager::neighbourPerturb(
    Solution &solution,
    SearchSpace &searchSpace,
    CostEvaluator const &costEvaluator) const
{
    size_t movesLeft = numPerturbations_;
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

void PerturbationManager::routePerturb(Solution &solution,
                                       SearchSpace &searchSpace,
                                       CostEvaluator const &costEvaluator) const
{
    if (searchSpace.activityOrder().empty())
        return;

    DynamicBitset perturbed
        = {solution.clients.size() + solution.shipments.size()};

    // Remove the seed and its successors from its route.
    auto const remove = [&](Route::Node *seed, size_t numMoves)
    {
        auto *route = seed->route();
        assert(route);

        std::vector<Route::Node *> selected = {seed};
        auto const seedIdx = seed->isClient()
                                 ? seed->idx()
                                 : solution.clients.size() + seed->idx();
        perturbed[seedIdx] = true;
        for (size_t offset = 1; offset != route->size(); ++offset)
        {
            if (selected.size() == numMoves)
                break;

            auto const pos = (seed->pos() + offset) % route->size();
            auto *node = (*route)[pos];
            if (!node->isClient() && !node->isShipment())
                continue;

            if (node->isDelivery())
                node = node - 1;  // pickup

            auto const idx = node->isClient()
                                 ? node->idx()
                                 : solution.clients.size() + node->idx();

            if (perturbed[idx])
                continue;

            perturbed[idx] = true;
            selected.push_back(node);
        }

        for (auto *node : selected)
        {
            searchSpace.markPromising(node);

            if (node->isPickup())
            {
                auto *delivery = node + 1;
                searchSpace.markPromising(delivery);
                assert(delivery->route() == route);
                route->remove(delivery->pos());
            }

            route->remove(node->pos());
        }

        route->update();

        if (route->empty())
            route->clear();
    };

    // Insert the seed and its unplanned neighbours into the same route.
    auto const insert = [&](Route::Node *seed, size_t numMoves)
    {
        assert(!seed->route());

        if (seed->isClient())
        {
            solution.insert(seed, searchSpace, costEvaluator, true);
            searchSpace.markPromising(seed);
        }
        else
        {
            assert(seed->isPickup());
            solution.insert(seed, seed + 1, searchSpace, costEvaluator, true);
            searchSpace.markPromising(seed);
            searchSpace.markPromising(seed + 1);
        }

        auto *route = seed->route();
        assert(route);
        route->update();

        size_t numInserted = 1;
        for (auto const &activity : searchSpace.neighboursOf(seed->activity()))
        {
            if (numInserted >= numMoves)
                break;

            auto *node = solution[activity];
            assert(node);

            if (node->isDelivery())
                node = node - 1;  // pickup

            auto const idx = node->isClient()
                                 ? node->idx()
                                 : solution.clients.size() + node->idx();
            if (perturbed[idx] || node->route())
                continue;

            if (node->isClient())
            {
                solution.insert(node, *route, costEvaluator);
                searchSpace.markPromising(node);
            }
            else
            {
                assert(node->isPickup());
                solution.insert(node, node + 1, *route, costEvaluator);
                searchSpace.markPromising(node);
                searchSpace.markPromising(node + 1);
            }

            route->update();
            perturbed[idx] = true;
            numInserted++;
        }
    };

    // Identify the seeds for route perturbation. We select the first node in
    // the activity order, and then go through its neighbours until we have
    // enough seeds. Seeds must be unplanned or in different routes.
    auto const numSeeds = std::min(numRoutes_, numPerturbations_);
    auto *first = solution[searchSpace.activityOrder().front()];
    assert(first);

    std::vector<Route::Node *> seeds = {first};
    std::vector<Route *> routes = {first->route()};
    auto const firstIdx = first->isClient()
                              ? first->idx()
                              : solution.clients.size() + first->idx();
    perturbed[firstIdx] = true;

    for (auto const &activity : searchSpace.neighboursOf(first->activity()))
    {
        if (seeds.size() == numSeeds)
            break;

        auto *node = solution[activity];
        assert(node);

        if (node->isDelivery())
            node = node - 1;  // pickup

        auto const idx = node->isClient()
                             ? node->idx()
                             : solution.clients.size() + node->idx();
        if (perturbed[idx])
            continue;

        auto *route = node->route();
        if (route
            && std::find(routes.begin(), routes.end(), route) != routes.end())
            continue;

        seeds.push_back(node);
        routes.push_back(route);
        perturbed[idx] = true;
    }

    auto const movesPerSeed = numPerturbations_ / seeds.size();
    auto const numExtraMoves = numPerturbations_ % seeds.size();

    for (size_t seedIdx = 0; seedIdx != seeds.size(); ++seedIdx)
    {
        auto const numMoves = movesPerSeed + (seedIdx < numExtraMoves);

        if (routes[seedIdx])
            remove(seeds[seedIdx], numMoves);
        else
            insert(seeds[seedIdx], numMoves);
    }
}

void PerturbationManager::perturb(Solution &solution,
                                  SearchSpace &searchSpace,
                                  CostEvaluator const &costEvaluator) const
{
    if (!numPerturbations_ || (useRoutePerturb_ && !numRoutes_))
        return;

    // Clear the set of promising nodes. Perturbation determines the initial
    // set of promising nodes for further (local search) improvement.
    searchSpace.unmarkAllPromising();

    if (useRoutePerturb_)
        routePerturb(solution, searchSpace, costEvaluator);
    else
        neighbourPerturb(solution, searchSpace, costEvaluator);
}
