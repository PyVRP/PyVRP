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

Route::Node *nodeFor(Solution &solution, Activity const &activity)
{
    auto *node = solution[activity];
    assert(node);
    return node->isDelivery() ? node - 1 : node;
}
}  // namespace

PerturbationParams::PerturbationParams(size_t minPerturbations,
                                       size_t maxPerturbations,
                                       size_t maxRoutes)
    : minPerturbations(minPerturbations),
      maxPerturbations(maxPerturbations),
      maxRoutes(maxRoutes)
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
    maxRoutes_ = 1 + rng.randint(params_.maxRoutes);
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

    auto const requestIdx = [&](Route::Node const *node)
    {
        assert(node->isClient() || node->isPickup());
        return node->isClient() ? node->idx()
                                : solution.clients.size() + node->idx();
    };
    auto const markPromising = [&](Route::Node *node)
    {
        searchSpace.markPromising(node);

        if (node->isPickup())
            searchSpace.markPromising(node + 1);
    };

    DynamicBitset perturbed
        = {solution.clients.size() + solution.shipments.size()};

    // Remove the seed and its successors from its route.
    auto const removePart = [&](Route::Node *seed, size_t numMoves)
    {
        auto *route = seed->route();
        assert(route);
        assert(numMoves > 0);

        std::vector<Route::Node *> selected = {seed};
        perturbed[requestIdx(seed)] = true;
        for (size_t offset = 1;
             offset != route->size() && selected.size() != numMoves;
             ++offset)
        {
            auto const pos = (seed->pos() + offset) % route->size();
            auto *candidate = (*route)[pos];
            if (!candidate->isClient() && !candidate->isShipment())
                continue;

            if (candidate->isDelivery())
                candidate = candidate - 1;  // pickup

            auto const candidateIdx = requestIdx(candidate);

            if (perturbed[candidateIdx])
                continue;

            perturbed[candidateIdx] = true;
            selected.push_back(candidate);
        }

        for (auto *candidate : selected)
        {
            markPromising(candidate);

            if (candidate->isPickup())
            {
                auto *delivery = candidate + 1;
                assert(delivery->route() == route);
                route->remove(delivery->pos());
            }

            route->remove(candidate->pos());
        }

        route->update();

        if (route->empty())
            route->clear();
    };

    // Insert the seed and its unplanned neighbours into the same route.
    auto const insertPart = [&](Route::Node *node, size_t numMoves)
    {
        assert(!node->route());

        if (node->isClient())
            solution.insert(node, searchSpace, costEvaluator, true);
        else
        {
            assert(node->isPickup());
            solution.insert(node, node + 1, searchSpace, costEvaluator, true);
        }

        auto *route = node->route();
        assert(route);
        route->update();
        markPromising(node);

        size_t numInserted = 1;
        for (auto const &activity : searchSpace.neighboursOf(node->activity()))
        {
            if (numInserted >= numMoves)
                break;

            auto *candidate = nodeFor(solution, activity);
            auto const candidateIdx = requestIdx(candidate);
            if (perturbed[candidateIdx] || candidate->route())
                continue;

            if (candidate->isClient())
                solution.insert(candidate, *route, costEvaluator);
            else
            {
                assert(candidate->isPickup());
                solution.insert(
                    candidate, candidate + 1, *route, costEvaluator);
            }

            route->update();
            markPromising(candidate);
            perturbed[candidateIdx] = true;
            numInserted++;
        }
    };

    // We select maxRoutes seed nodes if we can. The first follows the random
    // activity order, and the remaining seeds come from its neighbourhood.
    // Planned seeds must belong to distinct routes. We mark seeds immediately
    // so they cannot be consumed as part of another seed's route region.
    auto const maxRoutes = std::min(maxRoutes_, numPerturbations_);
    auto *first = nodeFor(solution, searchSpace.activityOrder().front());
    std::vector<Route::Node *> seeds = {first};
    perturbed[requestIdx(first)] = true;

    for (auto const &activity : searchSpace.neighboursOf(first->activity()))
    {
        if (seeds.size() == maxRoutes)
            break;

        auto *node = nodeFor(solution, activity);
        auto const idx = requestIdx(node);
        if (perturbed[idx])
            continue;

        auto *route = node->route();
        auto const hasRoute = [route](Route::Node const *seed)
        { return route && seed->route() == route; };
        if (std::any_of(seeds.begin(), seeds.end(), hasRoute))
            continue;

        seeds.push_back(node);
        perturbed[idx] = true;
    }

    auto const movesPerSeed = numPerturbations_ / seeds.size();
    auto const numExtraMoves = numPerturbations_ % seeds.size();
    auto const movesForSeed
        = [&](size_t idx) { return movesPerSeed + (idx < numExtraMoves); };

    // First remove route parts around planned seeds.
    for (size_t idx = 0; idx != seeds.size(); ++idx)
    {
        auto *seed = seeds[idx];
        if (!seed->route())
            continue;

        removePart(seed, movesForSeed(idx));
        seeds[idx] = nullptr;
    }

    // The remaining seeds were originally unplanned. Insert each one and its
    // neighbours into the same route.
    for (size_t idx = 0; idx != seeds.size(); ++idx)
        if (seeds[idx])
            insertPart(seeds[idx], movesForSeed(idx));
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
