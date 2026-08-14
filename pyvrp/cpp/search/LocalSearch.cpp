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

    ensureStructuralFeasibility(costEvaluator);
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
    for (int step = 0; !searchCompleted_; ++step)
    {
        PYVRP_DEBUG("pyvrp.search", "Entering search loop (step={}).", step);
        searchCompleted_ = true;

        for (auto const &uActivity : searchSpace_.activityOrder())
        {
            if (!searchSpace_.isPromising(uActivity))
                continue;

            auto *U = solution_[uActivity];
            assert(U);

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

            // Moves involving empty routes are not tested initially to avoid
            // using too many routes, but we will try it if we have not been
            // able to insert U yet (perhaps the solution is empty?).
            if (step >= 0 || !U->route())
                applyEmptyRouteMoves(U, costEvaluator);
        }
    }
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

void LocalSearch::ensureStructuralFeasibility(
    CostEvaluator const &costEvaluator)
{
    std::vector<size_t> groupCount(data.numGroups(), 0);  // tracks membership
    for (size_t idx = 0; idx != data.numGroups(); ++idx)  // count in solution
    {
        auto const &group = data.group(idx);
        for (auto const client : group)
            if (solution_.clients[client].route())
                groupCount[idx]++;
    }

    // Ensure all required clients, groups and shipments are present in the
    // solution.
    for (auto const &activity : searchSpace_.activityOrder())
    {
        switch (activity.type())
        {
        case Activity::ActivityType::CLIENT:
        {
            auto &node = solution_.clients[activity.idx()];
            auto const &client = data.client(activity.idx());

            if (!node.route() && client.required)  // must insert
            {
                solution_.insert(&node, searchSpace_, costEvaluator, true);
                update(node.route(), node.route());
                searchSpace_.markPromising(&node);
                continue;
            }

            if (client.group)
            {
                auto const idx = *client.group;
                auto const &group = data.group(idx);

                if (group.required && groupCount[idx] == 0)  // must insert
                {
                    assert(!node.route());
                    solution_.insert(&node, searchSpace_, costEvaluator, true);
                    update(node.route(), node.route());
                    searchSpace_.markPromising(&node);
                    groupCount[idx]++;
                    continue;
                }

                if (node.route() && groupCount[idx] > 1)  // must remove
                {
                    searchSpace_.markPromising(&node);
                    auto *route = node.route();
                    route->remove(node.pos());
                    update(route, route);
                    groupCount[idx]--;
                }
            }

            break;
        }

        case Activity::ActivityType::PICKUP:
        {
            auto const idx = activity.idx();
            auto &[pickup, delivery] = solution_.shipments[idx];
            auto const &shipment = data.shipment(idx);

            if (!pickup.route() && shipment.required)
            {
                solution_.insert(
                    &pickup, &delivery, searchSpace_, costEvaluator, true);
                update(pickup.route(), delivery.route());
                searchSpace_.markPromising(&pickup);
                searchSpace_.markPromising(&delivery);
            }

            break;
        }

        default:
            continue;
        }
    }

#ifndef NDEBUG
    // Debug checks to ensure we have restored structural feasibility.
    for (size_t idx = 0; idx != data.numClients(); ++idx)
    {
        auto const &node = solution_.clients[idx];
        auto const &clientData = data.client(idx);
        assert(node.route() || !clientData.required);
    }

    for (size_t idx = 0; idx != data.numGroups(); ++idx)
    {
        auto const &group = data.group(idx);
        assert(group.required ? groupCount[idx] == 1 : groupCount[idx] <= 1);
    }

    for (size_t idx = 0; idx != data.numShipments(); ++idx)
    {
        auto const &[pickup, delivery] = solution_.shipments[idx];
        auto const &shipment = data.shipment(idx);
        assert(pickup.route() == delivery.route());
        assert(pickup.route() || !shipment.required);
    }
#endif
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
