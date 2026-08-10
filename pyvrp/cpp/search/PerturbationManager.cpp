#include "PerturbationManager.h"

#include "DynamicBitset.h"

#include <algorithm>
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

    auto const insert = [&](Route::Node *node)
    {
        assert(node->isClient() || node->isPickup());
        assert(!node->route());

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
            solution.insert(pickup, delivery, searchSpace, costEvaluator, true);

            auto *route = pickup->route();
            assert(delivery->route() == route);

            route->update();
            searchSpace.markPromising(pickup);
            searchSpace.markPromising(delivery);
        }
    };

    auto const remove = [&](Route::Node *node)
    {
        assert(node->isClient() || node->isPickup());
        assert(node->route());

        searchSpace.markPromising(node);

        auto *route = node->route();
        route->remove(node->pos());

        if (node->isPickup())  // then we also remove the associated
        {                      // delivery node
            auto const *delivery = node + 1;
            assert(delivery->route() == route);

            searchSpace.markPromising(delivery);
            route->remove(delivery->pos());
        }

        route->update();
    };

    auto const numClients = solution.clients.size();
    DynamicBitset perturbed = {numClients + solution.shipments.size()};

    // Group nodes by their current route. Unplanned nodes share a nullptr
    // route.
    std::vector<std::pair<Route *, std::vector<Route::Node *>>> groups;
    auto const groupNode = [&](Route::Node *node)
    {
        assert(node->isClient() || node->isPickup());

        auto const idx
            = node->isClient() ? node->idx() : numClients + node->idx();
        if (perturbed[idx])
            return;

        perturbed[idx] = true;

        auto *route = node->route();
        auto const sameRoute
            = [route](auto const &group) { return group.first == route; };
        auto group = std::find_if(groups.begin(), groups.end(), sameRoute);

        if (group == groups.end())
            groups.push_back({route, {node}});
        else
            group->second.push_back(node);
    };

    // We perturb the local neighbourhood of randomly ordered activities,
    // grouped by their current route. Planned activities are removed and
    // unplanned ones are inserted.
    for (auto const &uActivity : searchSpace.activityOrder())
    {
        groups.clear();

        groupNode(solution[uActivity]);

        for (auto const &vActivity : searchSpace.neighboursOf(uActivity))
        {
            auto *node = solution[vActivity];
            assert(node);

            if (node->isDelivery())
                node = node - 1;  // pickup

            groupNode(node);
        }

        for (auto &[route, nodes] : groups)
            for (auto *node : nodes)
            {
                assert(node->route() == route);
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
