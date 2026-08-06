#include "PerturbationManager.h"

#include "DynamicBitset.h"

#include <cassert>
#include <stdexcept>

using pyvrp::search::PerturbationManager;
using pyvrp::search::PerturbationParams;
using pyvrp::search::Route;

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

    auto const numClients = solution.clients.size();
    DynamicBitset perturbed = {numClients + solution.shipments.size()};

    auto const perturb = [&](Route::Node *node)
    {
        assert(node->isClient() || node->isPickup());

        auto const idx = node->isPickup() * numClients + node->idx();
        if (perturbed[idx])
            return;

        auto *route = node->route();
        if (route)
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
        else
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

        perturbed[idx] = true;
        movesLeft--;
    };

    for (auto const &uActivity : searchSpace.activityOrder())
    {
        auto *U = solution[uActivity];
        assert(U);

        auto const *route = U->route();
        perturb(U);

        if (!movesLeft)
            return;

        // We either insert all unplanned nodes from U's neighbourhood if U
        // was itself unplanned, or remove neighbourhood nodes from U's route
        // if it is planned.
        for (auto const &vActivity : searchSpace.neighboursOf(uActivity))
        {
            auto *V = solution[vActivity];
            assert(V);

            if (V->isDelivery())
                V = V - 1;

            if (V->route() == route)
                perturb(V);

            if (!movesLeft)
                return;
        }
    }
}
