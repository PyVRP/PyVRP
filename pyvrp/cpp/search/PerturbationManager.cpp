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

    auto const numClients = solution.clients.size();
    DynamicBitset perturbed = {numClients + solution.shipments.size()};

    using RouteGroup = std::pair<Route *, std::vector<Route::Node *>>;
    auto const perturb = [&](Route::Node *node)
    {
        assert(node->isClient() || node->isPickup());

        auto *route = node->route();
        if (route)
        {
            searchSpace.markPromising(node);
            if (node->isPickup())
            {
                auto *delivery = node + 1;
                assert(delivery->route() == route);

                searchSpace.markPromising(delivery);
                route->remove(delivery->pos());
            }

            route->remove(node->pos());
        }
        else
        {
            assert(!node->route());

            if (node->isClient())
                solution.insert(node, searchSpace, costEvaluator, true);
            else
                solution.insert(
                    node, node + 1, searchSpace, costEvaluator, true);

            auto *insertedRoute = node->route();
            assert(insertedRoute);
            insertedRoute->update();
            searchSpace.markPromising(node);

            if (node->isPickup())
            {
                auto *delivery = node + 1;
                assert(delivery->route() == insertedRoute);
                searchSpace.markPromising(delivery);
            }
        }

        movesLeft--;
    };

    // We perturb the local neighbourhood of randomly ordered activities,
    // grouped by their current route. Unplanned activities share a null route
    // group. Planned activities are removed and unplanned ones are inserted.
    for (auto const &uActivity : searchSpace.activityOrder())
    {
        if (!movesLeft)
            break;

        std::vector<RouteGroup> groups;
        auto const addCandidate = [&](auto const &activity)
        {
            auto *node = solution[activity];
            if (!node)
                return;

            if (node->isDelivery())
                node = node - 1;  // pickup

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

        addCandidate(uActivity);
        for (auto const &vActivity : searchSpace.neighboursOf(uActivity))
            addCandidate(vActivity);

        for (auto &[route, nodes] : groups)
        {
            for (auto *node : nodes)
            {
                assert(node->route() == route);
                perturb(node);

                if (!movesLeft)
                    break;
            }

            if (route)
            {
                route->update();
                if (route->empty())
                    route->clear();
            }

            if (!movesLeft)
                return;
        }
    }
}
