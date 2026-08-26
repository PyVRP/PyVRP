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

void PerturbationManager::perturb(ProblemData const &data,
                                  Solution &solution,
                                  SearchSpace &searchSpace,
                                  CostEvaluator const &costEvaluator) const
{
    if (numPerturbations_ == 0)  // nothing to do
        return;

    // Clear the set of promising nodes. Perturbation determines the initial
    // set of promising nodes for further (local search) improvement.
    searchSpace.unmarkAllPromising();

    auto const insert = [&](Route::Node *node)  // insert and mark promising
    {
        assert(node->isClient() || node->isPickup());
        assert(!node->route());

        if (node->isClient())
        {
            // We cannot insert a client whose mutually exclusive group already
            // has another member in the solution.
            if (auto const &client = data.client(node->idx()); client.group)
                for (auto const member : data.group(*client.group))
                    if (solution.clients[member].route())
                        return false;

            solution.insert(node, searchSpace, costEvaluator, true);
            node->route()->update();
            searchSpace.markPromising(node);
        }
        else
        {
            auto *pickup = node;
            auto *delivery = node + 1;
            solution.insert(pickup, delivery, searchSpace, costEvaluator, true);

            auto *route = pickup->route();
            assert(delivery->route() == route);

            route->update();
            searchSpace.markPromising(pickup);
            searchSpace.markPromising(delivery);
        }

        return true;
    };

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
    // Planned activities are removed and unplanned ones are inserted. Only
    // successful removals and insertions count as perturbation.
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
                if (route)
                    remove(node);
                else if (!insert(node))
                    continue;

                movesLeft--;
                if (!movesLeft)
                    return;
            }
    }
}
