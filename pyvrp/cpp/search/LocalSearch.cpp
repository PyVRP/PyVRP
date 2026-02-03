#include "LocalSearch.h"
#include "DynamicBitset.h"
#include "Measure.h"
#include "Trip.h"
#include "primitives.h"

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
    std::fill(lastTest_.begin(), lastTest_.end(), -1);
    std::fill(lastUpdate_.begin(), lastUpdate_.end(), 0);
    numUpdates_ = 0;

    solution_.load(solution);

    for (auto *op : unaryOps_)
        op->init(solution);

    for (auto *op : binaryOps_)
        op->init(solution);

    if (exhaustive)
        searchSpace_.markAllPromising();
    else
        perturbationManager_.perturb(solution_, searchSpace_, costEvaluator);

    search(costEvaluator);

    return solution_.unload();
}

void LocalSearch::search(CostEvaluator const &costEvaluator)
{
    if (unaryOps_.empty() && binaryOps_.empty())
        return;

    searchCompleted_ = false;
    for (int step = 0; !searchCompleted_; ++step)
    {
        searchCompleted_ = true;

        for (auto const uClient : searchSpace_.clientOrder())
        {
            auto *U = &solution_.nodes[uClient];
            insertRequired(U, costEvaluator);

            if (!searchSpace_.isPromising(uClient))
                continue;

            auto const lastTest = lastTest_[U->client()];
            lastTest_[U->client()] = numUpdates_;

            applyUnaryOps(U, costEvaluator);

            // Evaluate moves involving the client's group, if it is in any.
            applyGroupMoves(U, costEvaluator);

            for (auto const vClient : searchSpace_.neighboursOf(U->client()))
            {
                auto *V = &solution_.nodes[vClient];

                if (!V->route())
                    continue;

                auto uIdx = std::distance(solution_.routes.data(), U->route());
                auto vIdx = std::distance(solution_.routes.data(), V->route());
                if (!U->route()
                    || std::max(lastUpdate_[uIdx], lastUpdate_[vIdx])
                           > lastTest)
                {
                    if (applyBinaryOps(U, V, costEvaluator))
                        continue;

                    if (p(V)->isStartDepot()
                        && applyBinaryOps(U, p(V), costEvaluator))
                        continue;
                }
            }

            if (!U->route())
                // Perhaps we can insert U after the first route's start
                // depot as a fallback.
                applyBinaryOps(U, solution_.routes[0][0], costEvaluator);

            // Moves involving empty routes are not tested in the first
            // iteration to avoid using too many routes.
            if (step > 0)
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
            auto *rU = U->route();

            [[maybe_unused]] auto const costBefore
                = costEvaluator.penalisedCost(*rU);

            searchSpace_.markPromising(U);

            op->apply(U);
            update(rU, rU);

            [[maybe_unused]] auto const costAfter
                = costEvaluator.penalisedCost(*rU);

            // When there is an improving move, the delta cost evaluation must
            // be exact. The resulting cost is then the sum of the cost before
            // the move, plus the delta cost.
            assert(costAfter == costBefore + deltaCost);

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
            auto *rU = U->route();
            if (rU)
                searchSpace_.markPromising(U);

            auto *rV = V->route();
            assert(rV);
            searchSpace_.markPromising(V);

            [[maybe_unused]] Cost costBefore = costEvaluator.penalisedCost(*rV);
            costBefore += rU && rU != rV ? costEvaluator.penalisedCost(*rU) : 0;

            op->apply(U, V);
            update(rU, rV);

            [[maybe_unused]] Cost costAfter = costEvaluator.penalisedCost(*rV);
            costAfter += rU && rU != rV ? costEvaluator.penalisedCost(*rU) : 0;

            // When there is an improving move, the delta cost evaluation must
            // be exact. The resulting cost is then the sum of the cost before
            // the move, plus the delta cost.
            assert(costAfter == costBefore + deltaCost);

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
    if (U->route())
        return;

    ProblemData::Client const &uData = data.location(U->client());

    if (uData.required)  // then we must insert U
    {
        solution_.insert(U, searchSpace_, costEvaluator, true);
        update(U->route(), U->route());
        searchSpace_.markPromising(U);
        return;
    }

    if (uData.group)
    {
        auto const &group = data.group(*uData.group);
        assert(group.mutuallyExclusive);

        if (!group.required)
            return;

        for (auto const client : group.clients())  // check if any of the group
            if (solution_.nodes[client].route())   // is already present - then
                return;                            // we need not insert

        if (solution_.insert(U, searchSpace_, costEvaluator, true))
        {
            update(U->route(), U->route());
            searchSpace_.markPromising(U);
        }
    }
}

void LocalSearch::applyGroupMoves(Route::Node *U,
                                  CostEvaluator const &costEvaluator)
{
    ProblemData::Client const &uData = data.location(U->client());

    if (!uData.group)
        return;

    auto const &group = data.group(*uData.group);
    assert(group.mutuallyExclusive);

    std::vector<size_t> inSol;
    auto const pred
        = [&](auto client) { return solution_.nodes[client].route(); };
    std::copy_if(group.begin(), group.end(), std::back_inserter(inSol), pred);

    if (inSol.empty())  // then it's not required, since if required we would
        return;         // have inserted U before.

    // We remove clients in order of increasing cost delta (biggest improvement
    // first), and evaluate swapping the last client with U.
    std::vector<Cost> costs;
    for (auto const client : inSol)
    {
        auto cost = removeCost(&solution_.nodes[client], data, costEvaluator);
        costs.push_back(cost);
    }

    // Sort clients in order of increasing removal costs.
    std::vector<size_t> range(inSol.size());
    std::iota(range.begin(), range.end(), 0);
    std::sort(range.begin(),
              range.end(),
              [&costs](auto idx1, auto idx2)
              { return costs[idx1] < costs[idx2]; });

    // Remove all but the last client, whose removal is the least valuable.
    for (auto idx = range.begin(); idx != range.end() - 1; ++idx)
    {
        auto const client = inSol[*idx];
        auto const &node = solution_.nodes[client];
        auto *route = node.route();

        searchSpace_.markPromising(&node);
        route->remove(node.idx());
        update(route, route);
    }

    // Test swapping U and V, and do so if U is better to have than V.
    auto *V = &solution_.nodes[inSol[range.back()]];
    if (U != V && inplaceCost(U, V, data, costEvaluator) < 0)
    {
        auto *route = V->route();
        auto const idx = V->idx();
        route->remove(idx);
        route->insert(idx, U);
        update(route, route);
        searchSpace_.markPromising(U);
        return;
    }

    // Test removing V if that's an improving and allowed move.
    if (!group.required && removeCost(V, data, costEvaluator) < 0)
    {
        searchSpace_.markPromising(V);
        auto *route = V->route();
        route->remove(V->idx());
        update(route, route);
        return;
    }
}

void LocalSearch::update(Route *U, Route *V)
{
    assert(V);
    numUpdates_++;
    searchCompleted_ = false;

    if (U)
    {
        U->update();
        lastUpdate_[std::distance(solution_.routes.data(), U)] = numUpdates_;
    }

    if (U != V)
    {
        V->update();
        lastUpdate_[std::distance(solution_.routes.data(), V)] = numUpdates_;
    }
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
      lastTest_(data.numLocations()),
      lastUpdate_(data.numVehicles())
{
}
