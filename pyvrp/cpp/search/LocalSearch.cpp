#include "LocalSearch.h"
#include "DynamicBitset.h"
#include "Measure.h"
#include "logging.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <numeric>

using pyvrp::Solution;
using pyvrp::search::BinaryOperator;
using pyvrp::search::LocalSearch;
using pyvrp::search::SearchSpace;
using pyvrp::search::UnaryOperator;

pyvrp::Solution LocalSearch::operator()(pyvrp::Solution const &solution,
                                        CostEvaluator const &costEvaluator,
                                        bool exhaustive)
{
    PYVRP_DEBUG(
        "pyvrp.search", "Applying local search (exhaustive={}).", exhaustive);

    std::fill(lastTest_.begin(), lastTest_.end(), -1);
    std::fill(lastUpdate_.begin(), lastUpdate_.end(), 0);
    numUpdates_ = 0;

    solution_.load(solution);

    for (auto *op : unaryOps_)
        op->init(solution_);

    for (auto *op : binaryOps_)
        op->init(solution_);

    if (exhaustive)
        searchSpace_.markAllPromising();
    else
        perturbationManager_.perturb(solution_, searchSpace_, costEvaluator);

    search(costEvaluator);

    [[maybe_unused]] auto const stats = statistics();
    PYVRP_DEBUG("pyvrp.search",
                "Completed local search: improving={}, updates={}, moves={}.",
                stats.numImproving,
                stats.numUpdates,
                stats.numMoves);

    return solution_.unload();
}

void LocalSearch::search(CostEvaluator const &costEvaluator)
{
    if (unaryOps_.empty() && binaryOps_.empty())
        return;

    searchCompleted_ = false;
    for ([[maybe_unused]] int step = 0; !searchCompleted_; ++step)
    {
        PYVRP_DEBUG("pyvrp.search", "Entering search loop (step={}).", step);
        searchCompleted_ = true;

        for (auto const &uActivity : searchSpace_.activityOrder())
        {
            auto *U = solution_[uActivity];
            assert(U);
            insertRequired(U, costEvaluator);

            if (!searchSpace_.isPromising(uActivity))
                continue;

            // lastTest_ is ordered - #clients (lower indices) and #pickups
            // (upper indices).
            auto const idx = (U->isClient() ? 0 : data.numClients()) + U->idx();
            auto const lastTest = lastTest_[idx];
            lastTest_[idx] = numUpdates_;

            applyUnaryOps(U, costEvaluator);

            for (auto const &vActivity : searchSpace_.neighboursOf(uActivity))
            {
                auto *V = solution_[vActivity];
                assert(V);

                if (!V->route())
                    continue;

                auto *routes = solution_.routes.data();
                auto uUpdate = 0;
                if (U->route())
                    uUpdate = lastUpdate_[std::distance(routes, U->route())];
                auto vUpdate = lastUpdate_[std::distance(routes, V->route())];
                if (uUpdate > lastTest || vUpdate > lastTest)
                {
                    if (applyBinaryOps(U, V, costEvaluator))
                        continue;

                    if (p(V)->isStartDepot()
                        && applyBinaryOps(U, p(V), costEvaluator))
                        continue;
                }
            }

            applyEmptyRouteMoves(U, costEvaluator);
        }
    }

#ifndef NDEBUG
    // Debug checks to ensure the search restored structural feasibility.
    for (size_t idx = 0; idx != data.numClients(); ++idx)
    {
        auto const &node = solution_.clients[idx];
        assert(node.route() || !data.client(idx).required);
    }

    for (auto const &group : data.groups())
    {
        auto const inSol = [&](auto member)
        { return solution_.clients[member].route() != nullptr; };
        auto const count = std::count_if(group.begin(), group.end(), inSol);
        assert(group.required ? count == 1 : count <= 1);
    }

    for (size_t idx = 0; idx != data.numShipments(); ++idx)
    {
        auto const &[pickup, delivery] = solution_.shipments[idx];
        assert(pickup.route() == delivery.route());
        assert(pickup.route() || !data.shipment(idx).required);
    }
#endif
}

void LocalSearch::shuffle(RandomNumberGenerator &rng)
{
    perturbationManager_.shuffle(rng);
    searchSpace_.shuffle(rng);

    rng.shuffle(unaryOps_.begin(), unaryOps_.end());
    rng.shuffle(binaryOps_.begin(), binaryOps_.end());
}

bool LocalSearch::applyUnaryOps(Route::Node *U,
                                CostEvaluator const &costEvaluator)
{
    for (auto *op : unaryOps_)
    {
        auto const [deltaCost, shouldApply] = op->evaluate(U, costEvaluator);
        if (shouldApply)
        {
            PYVRP_DEBUG("pyvrp.search",
                        "Applying operator {} to U={} (delta={}).",
                        op->name(),
                        U->idx(),
                        deltaCost);

            auto *rU = U->route();
            if (rU)
                searchSpace_.markPromising(U);

#ifndef NDEBUG
            auto const costBefore = costEvaluator.penalisedCost(solution_);
#endif

            op->apply(U);
            if (!rU)  // then U wasn't in the solution before, and the operator
            {         // just inserted it.
                rU = U->route();
                searchSpace_.markPromising(U);
            }

            update(rU, rU);

#ifndef NDEBUG
            auto const costAfter = costEvaluator.penalisedCost(solution_);
            // When there is an improving move, the delta cost evaluation must
            // be exact. The resulting cost is then the sum of the cost before
            // the move, plus the delta cost.
            assert(costAfter == costBefore + deltaCost);
#endif

            return true;
        }
    }

    return false;
}

bool LocalSearch::applyBinaryOps(Route::Node *U,
                                 Route::Node *V,
                                 CostEvaluator const &costEvaluator)
{
    for (auto *op : binaryOps_)
    {
        auto const [deltaCost, shouldApply] = op->evaluate(U, V, costEvaluator);
        if (shouldApply)
        {
            PYVRP_DEBUG("pyvrp.search",
                        "Applying operator {} to U={} and V={} (delta={}).",
                        op->name(),
                        U->idx(),
                        V->idx(),
                        deltaCost);

            auto *rU = U->route();
            auto *rV = V->route();
            assert(rV);

            if (rU)
                searchSpace_.markPromising(U);
            searchSpace_.markPromising(V);

#ifndef NDEBUG
            auto const costBefore = costEvaluator.penalisedCost(solution_);
#endif

            op->apply(U, V);
            update(rU, rV);

#ifndef NDEBUG
            auto const costAfter = costEvaluator.penalisedCost(solution_);
            // When there is an improving move, the delta cost evaluation must
            // be exact. The resulting cost is then the sum of the cost before
            // the move, plus the delta cost.
            assert(costAfter == costBefore + deltaCost);
#endif

            return true;
        }
    }

    return false;
}

void LocalSearch::applyEmptyRouteMoves(Route::Node *U,
                                       CostEvaluator const &costEvaluator)
{
    // We apply moves involving empty routes in the (randomised) order of
    // orderVehTypes. This helps because empty vehicle moves incur fixed cost,
    // and a purely greedy approach over-prioritises vehicles with low fixed
    // costs but possibly high variable costs.
    for (auto const &[vehType, offset] : searchSpace_.vehTypeOrder())
    {
        auto const begin = solution_.routes.begin() + offset;
        auto const end = begin + data.vehicleType(vehType).numAvailable;
        auto const pred = [](auto const &route) { return route.empty(); };
        auto empty = std::find_if(begin, end, pred);

        if (empty != end && applyBinaryOps(U, (*empty)[0], costEvaluator))
            break;
    }
}

void LocalSearch::insertRequired(Route::Node *U,
                                 CostEvaluator const &costEvaluator)
{
    assert(U->isClient() || U->isShipment());

    if (U->route())  // then U is planned, and there is nothing to do
        return;

    switch (U->type())
    {
    case Activity::ActivityType::DEPOT:
        return;

    case Activity::ActivityType::CLIENT:
    {
        auto const &client = data.client(U->idx());

        if (client.required)  // then we must insert U
        {
            solution_.insert(U, searchSpace_, costEvaluator, true);
            update(U->route(), U->route());
            searchSpace_.markPromising(U);
            return;
        }

        if (!client.group)
            return;

        auto const &group = data.group(*client.group);
        if (!group.required)
            return;

        for (auto const client : group.clients())   // check if any of the group
            if (solution_.clients[client].route())  // is already present - then
                return;                             // we need not insert

        solution_.insert(U, searchSpace_, costEvaluator, true);
        update(U->route(), U->route());
        searchSpace_.markPromising(U);
        return;
    }

    case Activity::ActivityType::PICKUP:
    case Activity::ActivityType::DELIVERY:
    {
        if (!data.shipment(U->idx()).required)
            return;

        auto &[pickup, delivery] = solution_.shipments[U->idx()];
        solution_.insert(&pickup, &delivery, searchSpace_, costEvaluator, true);
        update(pickup.route(), delivery.route());
        searchSpace_.markPromising(&pickup);
        searchSpace_.markPromising(&delivery);
        return;
    }
    }
}

void LocalSearch::update(Route *U, Route *V)
{
    assert(V);
    numUpdates_++;
    searchCompleted_ = false;

    auto const update = [&](Route *route)
    {
        route->update();
        if (route->empty())  // if route turned empty we clear it to remove any
            route->clear();  // lingering non-client nodes.

        auto const idx = std::distance(solution_.routes.data(), route);
        lastUpdate_[idx] = numUpdates_;

        for (auto *op : unaryOps_)   // some operators cache partial evaluations
            op->update(route);       // and rely on this call to keep those
        for (auto *op : binaryOps_)  // caches in sync.
            op->update(route);
    };

    if (U)
        update(U);

    if (U != V)
        update(V);
}

void LocalSearch::addOperator(UnaryOperator &op)
{
    unaryOps_.emplace_back(&op);
}

void LocalSearch::addOperator(BinaryOperator &op)
{
    binaryOps_.emplace_back(&op);
}

std::vector<UnaryOperator *> const &LocalSearch::unaryOperators() const
{
    return unaryOps_;
}

std::vector<BinaryOperator *> const &LocalSearch::binaryOperators() const
{
    return binaryOps_;
}

void LocalSearch::setNeighbours(SearchSpace::Neighbours neighbours)
{
    searchSpace_.setNeighbours(neighbours);
}

SearchSpace::Neighbours const &LocalSearch::neighbours() const
{
    return searchSpace_.neighbours();
}

LocalSearch::Statistics LocalSearch::statistics() const
{
    size_t numMoves = 0;
    size_t numImproving = 0;

    auto const count = [&](auto const *op)
    {
        auto const &stats = op->statistics();
        numMoves += stats.numEvaluations;
        numImproving += stats.numApplications;
    };

    std::for_each(unaryOps_.begin(), unaryOps_.end(), count);
    std::for_each(binaryOps_.begin(), binaryOps_.end(), count);

    assert(numImproving <= numUpdates_);
    return {numMoves, numImproving, numUpdates_};
}

LocalSearch::LocalSearch(ProblemData const &data,
                         SearchSpace::Neighbours neighbours,
                         PerturbationManager &perturbationManager)
    : data(data),
      solution_(data),
      searchSpace_(data, neighbours),
      perturbationManager_(perturbationManager),
      lastTest_(data.numClients() + data.numShipments()),
      lastUpdate_(data.numVehicles())
{
}
