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
    INSERT
};
}

PerturbationParams::PerturbationParams(size_t minPerturbations,
                                       size_t maxPerturbations,
                                       size_t maxRoutes,
                                       bool neighbouringRoutes)
    : minPerturbations(minPerturbations),
      maxPerturbations(maxPerturbations),
      maxRoutes(maxRoutes),
      neighbouringRoutes(neighbouringRoutes)
{
    if (minPerturbations > maxPerturbations)
        throw std::invalid_argument(
            "min_perturbations must be <= max_perturbations.");

    if (maxRoutes == 0)
        throw std::invalid_argument("max_routes must be positive.");
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
    useRoutePerturb_ = rng.randint(2) == 1;
    numRoutes_ = 1 + rng.randint(params_.maxRoutes);
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

    auto *seed = solution[searchSpace.activityOrder().front()];
    auto *route = seed->route();
    if (!route)
    {
        if (seed->isClient())
            solution.insert(seed, searchSpace, costEvaluator, true);
        else
        {
            assert(seed->isPickup());
            solution.insert(seed, seed + 1, searchSpace, costEvaluator, true);
        }

        route = seed->route();
        assert(route);
        route->update();

        searchSpace.markPromising(seed);

        if (seed->isPickup())
            searchSpace.markPromising(seed + 1);

        size_t movesLeft = numPerturbations_ - 1;

        for (auto const &activity : searchSpace.neighboursOf(seed->activity()))
        {
            if (!movesLeft)
                break;

            auto *node = solution[activity];
            if (node->isDelivery())
                node = node - 1;

            if (node->route())
                continue;

            if (node->isClient())
                solution.insert(node, *route, costEvaluator);
            else
            {
                assert(node->isPickup());
                solution.insert(node, node + 1, *route, costEvaluator);
            }

            route->update();
            searchSpace.markPromising(node);

            if (node->isPickup())
                searchSpace.markPromising(node + 1);

            movesLeft--;
        }

        return;
    }

    std::vector<Route *> routes = {route};
    std::vector<Route::Node *> seeds = {seed};
    auto const numRoutes = std::min(numRoutes_, numPerturbations_);
    auto const &activities = params_.neighbouringRoutes
                                 ? searchSpace.neighboursOf(seed->activity())
                                 : searchSpace.activityOrder();

    for (auto const &activity : activities)
    {
        if (routes.size() == numRoutes)
            break;

        auto *node = solution[activity];
        auto *candidateRoute = node->route();
        if (!candidateRoute)
            continue;

        if (std::find(routes.begin(), routes.end(), candidateRoute)
            != routes.end())
            continue;

        routes.push_back(candidateRoute);
        seeds.push_back(node);
    }

    auto const movesPerRoute = numPerturbations_ / routes.size();
    auto const numExtraMoves = numPerturbations_ % routes.size();
    DynamicBitset perturbed
        = {solution.clients.size() + solution.shipments.size()};

    for (size_t idx = 0; idx != routes.size(); ++idx)
    {
        auto *route = routes[idx];
        auto *seed = seeds[idx];
        auto const numMoves = movesPerRoute + (idx < numExtraMoves);

        std::vector<Route::Node *> selected;
        for (size_t offset = 0; offset != route->size(); ++offset)
        {
            auto const pos = (seed->pos() + offset) % route->size();
            auto *candidate = (*route)[pos];
            if (!candidate->isClient() && !candidate->isShipment())
                continue;

            if (candidate->isDelivery())
                candidate = candidate - 1;  // pickup

            auto const idx = candidate->isClient()
                                 ? candidate->idx()
                                 : solution.clients.size() + candidate->idx();

            if (perturbed[idx])
                continue;

            perturbed[idx] = true;
            selected.push_back(candidate);
            if (selected.size() == numMoves)
                break;
        }

        for (auto *candidate : selected)
        {
            searchSpace.markPromising(candidate);

            if (candidate->isPickup())
            {
                auto *delivery = candidate + 1;
                assert(delivery->route() == route);

                searchSpace.markPromising(delivery);
                route->remove(delivery->pos());
            }

            route->remove(candidate->pos());
        }

        route->update();

        if (route->empty())
            route->clear();
    }
}

void PerturbationManager::perturb(Solution &solution,
                                  SearchSpace &searchSpace,
                                  CostEvaluator const &costEvaluator) const
{
    if (!numPerturbations_)
        return;

    // Clear the set of promising nodes. Perturbation determines the initial
    // set of promising nodes for further (local search) improvement.
    searchSpace.unmarkAllPromising();

    if (useRoutePerturb_)
        routePerturb(solution, searchSpace, costEvaluator);
    else
        neighbourPerturb(solution, searchSpace, costEvaluator);
}
