#include "PerturbationManager.h"

#include "DynamicBitset.h"

#include <cassert>
#include <stdexcept>
#include <utility>
#include <vector>

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

PerturbationManager::PerturbationManager(ProblemData const &data,
                                         PerturbationParams params)
    : data_(data), params_(params), numPerturbations_(params_.minPerturbations)
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
    if (numPerturbations_ == 0)  // nothing to do
        return;

    // Clear the set of promising nodes. Perturbation determines the initial
    // set of promising nodes for further (local search) improvement.
    searchSpace.unmarkAllPromising();

    auto const remove = [&](Route::Node *node)  // remove and mark promising
    {
        assert(node->isClient() || node->isPickup());
        assert(node->route());

        searchSpace.markPromising(node);

        auto *route = node->route();
        route->remove(node->pos());

        if (node->isPickup())  // then we also remove the associated delivery
        {
            auto const *delivery = node + 1;
            assert(delivery->route() == route);

            searchSpace.markPromising(delivery);
            route->remove(delivery->pos());
        }

        route->update();
    };

    auto const insert = [&](Route::Node *node)  // insert and mark promising
    {
        assert(node->isClient() || node->isPickup());
        assert(!node->route());

        auto const group = node->isClient() ? data_.client(node->idx()).group
                                            : data_.shipment(node->idx()).group;

        if (group)
            // Remove the planned group member before inserting its replacement.
            for (auto const &activity : data_.group(*group))
            {
                auto *other = solution[activity];
                if (other->route())
                {
                    remove(other);
                    break;
                }
            }

        if (node->isClient())
            solution.insert(node, searchSpace, costEvaluator, true);
        else
            solution.insert(node, node + 1, searchSpace, costEvaluator, true);

        node->route()->update();
        searchSpace.markPromising(node);

        if (node->isPickup())
            searchSpace.markPromising(node + 1);
    };

    DynamicBitset perturbed
        = {solution.clients.size() + solution.shipments.size()};

    std::vector<std::pair<Route *, std::vector<Route::Node *>>> groups;
    auto const group = [&](Route::Node *node)  // group nodes by their route
    {
        assert(node && (node->isClient() || node->isPickup()));
        auto const idx = node->isClient()
                             ? node->idx()
                             : solution.clients.size() + node->idx();

        // This node has already been touched by a previous iteration, so
        // we skip it here.
        if (perturbed[idx])
            return;

        perturbed[idx] = true;

        for (auto &[groupRoute, groupNodes] : groups)  // insert into existing
            if (groupRoute == node->route())           // route group..
            {
                groupNodes.push_back(node);
                return;
            }

        groups.push_back({node->route(), {node}});  // .. or create new group
    };

    // We perturb the local neighbourhood of randomly ordered activities,
    // grouped by their current route (unplanned by their nullptr route).
    // Planned activities are removed and unplanned ones are inserted.
    for (size_t movesLeft = numPerturbations_;
         auto const &uActivity : searchSpace.activityOrder())
    {
        groups.clear();

        auto *node = solution[uActivity];
        group(node);

        for (auto const &vActivity : searchSpace.neighboursOf(uActivity))
        {
            auto *node = solution[vActivity];  // only group clients and pickups
            group(node->isDelivery() ? node - 1 : node);
        }

        for (auto &[route, nodes] : groups)
            for (auto *node : nodes)
            {
                if (route && !node->route())  // already removed by an earlier
                    continue;                 // mutually exclusive group swap

                if (route)
                    remove(node);
                else
                    insert(node);

                movesLeft--;
                if (!movesLeft)
                    return;
            }
    }
}
