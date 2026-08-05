#include "PerturbationManager.h"

#include "DynamicBitset.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>
#include <vector>

using pyvrp::Activity;
using pyvrp::search::PerturbationManager;
using pyvrp::search::PerturbationParams;
using pyvrp::search::Route;
using pyvrp::search::Solution;

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

    using RouteBucket = std::pair<Route *, std::vector<Route::Node *>>;

    // We perturb the local neighbourhood of randomly ordered activities,
    // grouped by their current route. Unplanned activities share a null route
    // bucket. Planned activities are removed and unplanned ones are inserted.
    for (auto const &uActivity : searchSpace.activityOrder())
    {
        if (!movesLeft)
            break;

        std::vector<RouteBucket> buckets;
        DynamicBitset candidates = {numClients + solution.shipments.size()};
        auto const addCandidate = [&](Activity const &activity)
        {
            auto *node = solution[activity];
            assert(node);

            if (node->isDelivery())
                node = node - 1;  // pickup

            auto const idx
                = node->isClient() ? node->idx() : numClients + node->idx();
            auto *route = node->route();
            if (perturbed[idx] || candidates[idx])
                return;

            auto const sameRoute
                = [route](auto const &bucket) { return bucket.first == route; };
            auto bucket
                = std::find_if(buckets.begin(), buckets.end(), sameRoute);

            if (bucket == buckets.end())
                buckets.push_back({route, {node}});
            else
                bucket->second.push_back(node);

            candidates[idx] = true;
        };

        addCandidate(uActivity);
        for (auto const &vActivity : searchSpace.neighboursOf(uActivity))
            addCandidate(vActivity);

        for (auto &[route, nodes] : buckets)
        {
            if (!movesLeft)
                break;

            if (!route)
            {
                for (auto *node : nodes)
                {
                    if (!movesLeft)
                        break;

                    assert(!node->route());

                    auto const inserted
                        = node->isClient()
                              ? solution.insert(
                                    node, searchSpace, costEvaluator, true)
                              : solution.insert(node,
                                                node + 1,
                                                searchSpace,
                                                costEvaluator,
                                                true);

                    if (!inserted)
                        continue;

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

                    auto const idx = node->isClient()
                                         ? node->idx()
                                         : numClients + node->idx();
                    perturbed[idx] = true;
                    movesLeft--;
                }

                continue;
            }

            for (auto *node : nodes)
            {
                if (!movesLeft)
                    break;

                assert(node->route() == route);

                searchSpace.markPromising(node);
                if (node->isPickup())
                {
                    auto *delivery = node + 1;
                    assert(delivery->route() == route);

                    searchSpace.markPromising(delivery);
                    route->remove(delivery->pos());
                }

                route->remove(node->pos());
                auto const idx
                    = node->isClient() ? node->idx() : numClients + node->idx();
                perturbed[idx] = true;
                movesLeft--;
            }

            route->update();
            if (route->empty())
                route->clear();
        }
    }
}
