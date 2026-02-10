#include "LocalSearch.h"
#include "DynamicBitset.h"
#include "Measure.h"
#include "Trip.h"

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
        op->init(solution_);

    for (auto *op : binaryOps_)
        op->init(solution_);

    if (exhaustive)
        searchSpace_.markAllPromising();
    else
        perturbationManager_.perturb(solution_, searchSpace_, costEvaluator);

    ensureStructuralFeasibility(costEvaluator);
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
            if (!searchSpace_.isPromising(uClient))
                continue;

            auto const lastTest = lastTest_[U->client()];
            lastTest_[U->client()] = numUpdates_;

            applyUnaryOps(U, costEvaluator);

            for (auto const vClient : searchSpace_.neighboursOf(U->client()))
            {
                auto *V = &solution_.nodes[vClient];

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
            if (step > 0 || !U->route())
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
            if (rU)
                searchSpace_.markPromising(U);

            [[maybe_unused]] auto const costBefore
                = costEvaluator.penalisedCost(solution_);

            op->apply(U);
            if (!rU)  // then U wasn't in the solution before, and the operator
            {         // just inserted it.
                rU = U->route();
                searchSpace_.markPromising(U);
            }

            update(rU, rU);

            [[maybe_unused]] auto const costAfter
                = costEvaluator.penalisedCost(solution_);

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
            auto *rV = V->route();
            assert(rV);

            if (rU)
                searchSpace_.markPromising(U);
            searchSpace_.markPromising(V);

            [[maybe_unused]] auto const costBefore
                = costEvaluator.penalisedCost(solution_);

            op->apply(U, V);
            update(rU, rV);

            [[maybe_unused]] auto const costAfter
                = costEvaluator.penalisedCost(solution_);

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

void LocalSearch::ensureStructuralFeasibility(
    CostEvaluator const &costEvaluator)
{
    std::vector<size_t> groupCount(data.numGroups(), 0);  // tracks membership
    for (size_t idx = 0; idx != data.numGroups(); ++idx)  // count in solution
    {
        auto const &group = data.group(idx);
        for (auto const client : group)
            if (solution_.nodes[client].route())
                groupCount[idx]++;
    }

    // Ensure all required clients and groups are present in the solution.
    for (auto const client : searchSpace_.clientOrder())
    {
        auto &node = solution_.nodes[client];
        ProblemData::Client const &clientData = data.location(client);

        if (!node.route() && clientData.required)  // then we must insert
        {
            solution_.insert(&node, searchSpace_, costEvaluator, true);
            update(node.route(), node.route());
            searchSpace_.markPromising(&node);
            continue;
        }

        if (clientData.group)
        {
            auto const idx = *clientData.group;
            auto const &group = data.group(idx);

            if (group.required && groupCount[idx] == 0)  // then we must insert
            {
                assert(!node.route());
                solution_.insert(&node, searchSpace_, costEvaluator, true);
                update(node.route(), node.route());
                searchSpace_.markPromising(&node);
                groupCount[idx]++;
                continue;
            }

            if (node.route() && groupCount[idx] > 1)  // then we must remove
            {
                searchSpace_.markPromising(&node);
                auto *route = node.route();
                route->remove(node.idx());
                update(route, route);
                groupCount[idx]--;
            }
        }
    }

#ifndef NDEBUG
    // Debug checks to ensure we have restored structural feasibility.
    for (size_t idx = data.numDepots(); idx != data.numLocations(); ++idx)
    {
        auto const &node = solution_.nodes[idx];
        ProblemData::Client const &clientData = data.location(idx);
        assert(node.route() || !clientData.required);
    }

    for (size_t idx = 0; idx != data.numGroups(); ++idx)
    {
        auto const &group = data.group(idx);
        assert(group.required ? groupCount[idx] == 1 : groupCount[idx] <= 1);
    }
#endif
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
