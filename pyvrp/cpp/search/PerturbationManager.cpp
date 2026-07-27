#include "PerturbationManager.h"

#include "DynamicBitset.h"

#include <cassert>
#include <stdexcept>

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
            if (node->isClient())
            {
                searchSpace.markPromising(node);
                route->remove(node->pos());
                movesLeft--;
            }
            else if (node->isPickup() && movesLeft > 1)
            {
                auto *pickup = node;
                auto *delivery = node + 1;
                assert(delivery->route() == route);

                searchSpace.markPromising(pickup);
                searchSpace.markPromising(delivery);
                route->remove(delivery->pos());
                route->remove(pickup->pos());
                movesLeft -= 2;
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
                movesLeft--;
            }
            else if (node->isPickup() && movesLeft > 1)
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
                movesLeft -= 2;
            }
        }
        else  // no-op
            return;

        perturbed[idx] = true;
    };

    // We do numPerturbations if we can. We perturb the local neighbourhood of
    // a randomly selected clients or pickups: if a selected client or pickup U
    // is in the solution, we remove it and its neighbours V. If it is not, we
    // try to insert instead. Client perturbations count as one move, while
    // shipment perturbations count as two.
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
