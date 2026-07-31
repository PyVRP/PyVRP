#include "PerturbationManager.h"

#include "DynamicBitset.h"

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
                                       SearchSpace &searchSpace) const
{
    size_t movesLeft = numPerturbations_;
    DynamicBitset perturbed
        = {solution.clients.size() + solution.shipments.size()};

    for (auto const &uActivity : searchSpace.activityOrder())
    {
        if (!movesLeft)
            return;

        auto *node = solution[uActivity];
        auto *route = node->route();
        if (!route)
            continue;

        std::vector<Route::Node *> selected;
        for (size_t offset = 0; offset != route->size(); ++offset)
        {
            auto const pos = (node->pos() + offset) % route->size();
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
            if (selected.size() == movesLeft)
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

        movesLeft -= selected.size();
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

    if (routeRemoval_)
        routePerturb(solution, searchSpace);
    else
        neighbourPerturb(solution, searchSpace, costEvaluator);
}
