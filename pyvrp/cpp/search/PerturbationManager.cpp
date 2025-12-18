#include "PerturbationManager.h"
#include "primitives.h"

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
                                  ProblemData const &data,
                                  CostEvaluator const &costEvaluator) const
{
    size_t movesLeft = numPerturbations_;

    if (!movesLeft)  // nothing to do
        return;

    // Clear the set of promising nodes. Perturbation determines the initial
    // set of promising nodes for further (local search) improvement.
    searchSpace.unmarkAllPromising();

    DynamicBitset perturbed = {solution.nodes.size()};
    auto const perturb = [&](auto *node, PerturbType action)
    {
        // This node has already been touched by a previous perturbation, so
        // we skip it here.
        if (perturbed[node->client()])
            return;

        // Remove if node is in a route and we are currently removing.
        auto *route = node->route();
        if (route && action == PerturbType::REMOVE)
        {
            searchSpace.markPromising(node);
            route->remove(node->idx());
            route->update();
        }
        // Insert if node is not in a route and we are currently inserting.
        else if (!route && action == PerturbType::INSERT)
        {
            insert(node, solution, searchSpace, data, costEvaluator);
            searchSpace.markPromising(node);
        }
        else  // no-op
            return;

        perturbed[node->client()] = true;
        movesLeft--;
    };

    // We do numPerturbations if we can. We perturb the local neighbourhood of
    // randomly selected clients U: if U is in the solution, we remove it and
    // its neighbours, while if it is not, we try to insert instead. Each
    // removal or insertion counts as one perturbation.
    for (auto const uClient : searchSpace.clientOrder())
    {
        auto *U = &solution.nodes[uClient];
        auto action = U->route() ? PerturbType::REMOVE : PerturbType::INSERT;
        perturb(U, action);

        if (!movesLeft)
            return;

        for (auto const vClient : searchSpace.neighboursOf(U->client()))
        {
            auto *V = &solution.nodes[vClient];
            perturb(V, action);

            if (!movesLeft)
                return;
        }
    }
}

void PerturbationManager::insert(Route::Node *U,
                                 Solution &solution,
                                 SearchSpace const &searchSpace,
                                 ProblemData const &data,
                                 CostEvaluator const &costEvaluator) const
{
    assert(!U->isDepot());
    Route::Node *UAfter = solution.routes[0][0];
    auto bestCost = insertCost(U, UAfter, data, costEvaluator);

    for (auto const vClient : searchSpace.neighboursOf(U->client()))
    {
        auto *V = &solution.nodes[vClient];

        if (!V->route())
            continue;

        auto const cost = insertCost(U, V, data, costEvaluator);
        if (cost < bestCost)
        {
            bestCost = cost;
            UAfter = V;
        }
    }

    for (auto const &[vehType, offset] : searchSpace.vehTypeOrder())
    {
        auto const begin = solution.routes.begin() + offset;
        auto const end = begin + data.vehicleType(vehType).numAvailable;
        auto const pred = [](auto const &route) { return route.empty(); };
        auto empty = std::find_if(begin, end, pred);

        if (empty == end)
            continue;

        auto const cost = insertCost(U, (*empty)[0], data, costEvaluator);
        if (cost < bestCost)
        {
            empty->insert(1, U);
            empty->update();
            return;
        }
    }

    auto *route = UAfter->route();
    route->insert(UAfter->idx() + 1, U);
    route->update();
}
