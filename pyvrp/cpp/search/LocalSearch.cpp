#include "LocalSearch.h"
#include "DynamicBitset.h"
#include "Measure.h"
#include "Trip.h"
#include "primitives.h"

#include <algorithm>
#include <cassert>
#include <numeric>

using pyvrp::Solution;
using pyvrp::search::LocalSearch;
using pyvrp::search::NodeOperator;
using pyvrp::search::RouteOperator;

Solution LocalSearch::operator()(Solution const &solution,
                                 CostEvaluator const &costEvaluator,
                                 std::vector<size_t> const &candidateNodes)
{
    numEvaluations = 0;
    loadSolution(solution, data, routes, nodes);

    candidates.reset();
    for (auto idx : candidateNodes)
        candidates[idx] = true;

    search(costEvaluator);
    intensify(costEvaluator);

    return exportSolution(routes, data);
}

Solution LocalSearch::search(Solution const &solution,
                             CostEvaluator const &costEvaluator)
{
    loadSolution(solution, data, routes, nodes);
    search(costEvaluator);
    return exportSolution(routes, data);
}

Solution LocalSearch::intensify(Solution const &solution,
                                CostEvaluator const &costEvaluator)
{
    loadSolution(solution, data, routes, nodes);
    intensify(costEvaluator);
    return exportSolution(routes, data);
}

void LocalSearch::search(CostEvaluator const &costEvaluator)
{
    if (nodeOps.empty())
        return;

    // Caches the last time nodes were tested for modification (uses numMoves to
    // track this).
    std::vector<int> lastTestedNodes(data.numLocations(), -1);

    bool firstStep = true;
    while (candidates.any())
    {
        // Node operators are evaluated for neighbouring (U, V) pairs.
        for (auto const uClient : orderNodes)
        {
            auto *U = &nodes[uClient];

            auto const lastTested = lastTestedNodes[uClient];
            lastTestedNodes[uClient] = numUpdates_;

            // First test removing or inserting U. Particularly relevant if not
            // all clients are required (e.g., when prize collecting).
            applyOptionalClientMoves(U, costEvaluator);

            // Evaluate moves involving the client's group, if it is in any.
            applyGroupMoves(U, costEvaluator);

            // We already evaluated inserting U, so there is nothing left to
            // be done for this client. We also remove it as candidate.
            if (!U->route())
            {
                candidates[uClient] = false;
                continue;
            }

            // We next evaluate node operators that work on pairs of nodes
            // of nodes (U, V), where both U and V are in the solution. We
            // only do this if U is a promising candidate for improvement.
            if (!candidates[uClient])
                continue;

            candidates[uClient] = false;

            // If U borders a reload depot, try removing it.
            applyDepotRemovalMove(p(U), costEvaluator);
            applyDepotRemovalMove(n(U), costEvaluator);

            // We next apply the regular node operators. These work on pairs
            // of nodes (U, V), where both U and V are in the solution.
            for (auto const vClient : neighbours_[uClient])
            {
                auto *V = &nodes[vClient];

                if (!V->route())
                    continue;

                if (lastUpdated[U->route()->idx()] > lastTested
                    || lastUpdated[V->route()->idx()] > lastTested)
                {
                    if (applyNodeOps(U, V, costEvaluator))
                        continue;

                    if (p(V)->isStartDepot()
                        && applyNodeOps(U, p(V), costEvaluator))
                        continue;
                }
            }

            // Moves involving empty routes are not tested in the first
            // iteration to avoid using too many routes.
            if (!firstStep)
                applyEmptyRouteMoves(U, costEvaluator);
        }

        firstStep = false;
    }
}

void LocalSearch::intensify(CostEvaluator const &costEvaluator)
{
    if (routeOps.empty())
        return;

    searchCompleted_ = false;
    while (!searchCompleted_)
    {
        searchCompleted_ = true;

        for (auto const rU : orderRoutes)
        {
            auto &U = routes[rU];
            assert(U.idx() == rU);

            if (U.empty())
                continue;

            auto const lastTested = lastTestedRoutes[U.idx()];
            lastTestedRoutes[U.idx()] = numUpdates_;

            for (size_t rV = U.idx() + 1; rV != routes.size(); ++rV)
            {
                auto &V = routes[rV];
                assert(V.idx() == rV);

                if (V.empty())
                    continue;

                if (lastUpdated[U.idx()] > lastTested
                    || lastUpdated[V.idx()] > lastTested)
                    applyRouteOps(&U, &V, costEvaluator);
            }
        }
    }
}

void LocalSearch::shuffle(RandomNumberGenerator &rng)
{
    std::shuffle(orderNodes.begin(), orderNodes.end(), rng);
    std::shuffle(nodeOps.begin(), nodeOps.end(), rng);

    std::shuffle(orderRoutes.begin(), orderRoutes.end(), rng);
    std::shuffle(routeOps.begin(), routeOps.end(), rng);

    std::shuffle(orderVehTypes.begin(), orderVehTypes.end(), rng);
}

bool LocalSearch::applyNodeOps(Route::Node *U,
                               Route::Node *V,
                               CostEvaluator const &costEvaluator)
{
    for (auto *nodeOp : nodeOps)
    {
        auto const deltaCost = nodeOp->evaluate(U, V, costEvaluator);
        numEvaluations += 1;

        if (deltaCost < 0)
        {
            auto *rU = U->route();  // copy these because the operator can
            auto *rV = V->route();  // modify the nodes' route membership

            [[maybe_unused]] auto const costBefore
                = costEvaluator.penalisedCost(*rU)
                  + Cost(rU != rV) * costEvaluator.penalisedCost(*rV);

            nodeOp->apply(U, V);
            update(rU, rV);

            if (!U->isDepot())
                candidates[U->client()] = true;

            if (!p(U)->isDepot())
                candidates[p(U)->client()] = true;

            if (!n(U)->isDepot())
                candidates[n(U)->client()] = true;

            if (!V->isDepot())
            {
                candidates[V->client()] = true;

                if (!p(V)->isDepot())
                    candidates[p(V)->client()] = true;

                if (!n(V)->isDepot())
                    candidates[n(V)->client()] = true;
            }

            [[maybe_unused]] auto const costAfter
                = costEvaluator.penalisedCost(*rU)
                  + Cost(rU != rV) * costEvaluator.penalisedCost(*rV);

            // When there is an improving move, the delta cost evaluation must
            // be exact. The resulting cost is then the sum of the cost before
            // the move, plus the delta cost.
            assert(costAfter == costBefore + deltaCost);

            return true;
        }
    }

    return false;
}

bool LocalSearch::applyRouteOps(Route *U,
                                Route *V,
                                CostEvaluator const &costEvaluator)
{
    for (auto *routeOp : routeOps)
    {
        auto const deltaCost = routeOp->evaluate(U, V, costEvaluator);
        if (deltaCost < 0)
        {
            [[maybe_unused]] auto const costBefore
                = costEvaluator.penalisedCost(*U)
                  + Cost(U != V) * costEvaluator.penalisedCost(*V);

            routeOp->apply(U, V);
            update(U, V);

            [[maybe_unused]] auto const costAfter
                = costEvaluator.penalisedCost(*U)
                  + Cost(U != V) * costEvaluator.penalisedCost(*V);

            // When there is an improving move, the delta cost evaluation must
            // be exact. The resulting cost is then the sum of the cost before
            // the move, plus the delta cost.
            assert(costAfter == costBefore + deltaCost);

            return true;
        }
    }

    return false;
}

void LocalSearch::applyDepotRemovalMove(Route::Node *U,
                                        CostEvaluator const &costEvaluator)
{
    if (!U->isReloadDepot())
        return;

    // We remove the depot when that's either better, or neutral. It can be
    // neutral if for example it's the same depot visited consecutively, but
    // that's then unnecessary.
    if (removeCost(U, data, costEvaluator) <= 0)
    {
        auto *route = U->route();
        route->remove(U->idx());
        update(route, route);
    }
}

void LocalSearch::applyEmptyRouteMoves(Route::Node *U,
                                       CostEvaluator const &costEvaluator)
{
    assert(U->route());

    // We apply moves involving empty routes in the (randomised) order of
    // orderVehTypes. This helps because empty vehicle moves incur fixed cost,
    // and a purely greedy approach over-prioritises vehicles with low fixed
    // costs but possibly high variable costs.
    for (auto const &[vehType, offset] : orderVehTypes)
    {
        auto const begin = routes.begin() + offset;
        auto const end = begin + data.vehicleType(vehType).numAvailable;
        auto const pred = [](auto const &route) { return route.empty(); };
        auto empty = std::find_if(begin, end, pred);

        if (empty != end && applyNodeOps(U, (*empty)[0], costEvaluator))
            break;
    }
}

void LocalSearch::applyOptionalClientMoves(Route::Node *U,
                                           CostEvaluator const &costEvaluator)
{
    ProblemData::Client const &uData = data.location(U->client());

    if (uData.group)  // groups have their own operator - applyGroupMoves()
        return;

    if (!uData.required && removeCost(U, data, costEvaluator) < 0)
    {
        auto *route = U->route();
        route->remove(U->idx());
        update(route, route);
    }

    if (!U->route())
        insert(U, costEvaluator, uData.required);
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
    auto const pred = [&](auto client) { return nodes[client].route(); };
    std::copy_if(group.begin(), group.end(), std::back_inserter(inSol), pred);

    if (inSol.empty())
    {
        insert(U, costEvaluator, group.required);
        return;
    }

    // We remove clients in order of increasing cost delta (biggest improvement
    // first), and evaluate swapping the last client with U.
    std::vector<Cost> costs;
    for (auto const client : inSol)
        costs.push_back(removeCost(&nodes[client], data, costEvaluator));

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
        auto const &node = nodes[client];
        auto *route = node.route();

        route->remove(node.idx());
        update(route, route);
    }

    // Test swapping U and V, and do so if U is better to have than V.
    auto *V = &nodes[inSol[range.back()]];
    if (U != V && inplaceCost(U, V, data, costEvaluator) < 0)
    {
        auto *route = V->route();
        auto const idx = V->idx();
        route->remove(idx);
        route->insert(idx, U);
        update(route, route);
    }
}

void LocalSearch::insert(Route::Node *U,
                         CostEvaluator const &costEvaluator,
                         bool required)
{
    Route::Node *UAfter = routes[0][0];
    Cost bestCost = insertCost(U, UAfter, data, costEvaluator);

    for (auto const vClient : neighbours_[U->client()])
    {
        auto *V = &nodes[vClient];

        if (!V->route())
            continue;

        auto const cost = insertCost(U, V, data, costEvaluator);
        if (cost < bestCost)
        {
            bestCost = cost;
            UAfter = V;
        }
    }

    if (required || bestCost < 0)
    {
        UAfter->route()->insert(UAfter->idx() + 1, U);
        update(UAfter->route(), UAfter->route());
    }
}

void LocalSearch::update(Route *U, Route *V)
{
    numUpdates_++;
    searchCompleted_ = false;

    U->update();
    lastUpdated[U->idx()] = numUpdates_;

    for (auto *op : routeOps)  // this is used by some route operators
        op->update(U);         // to keep caches in sync.

    if (U != V)
    {
        V->update();
        lastUpdated[V->idx()] = numUpdates_;

        for (auto *op : routeOps)  // this is used by some route operators
            op->update(V);         // to keep caches in sync.
    }
}

void LocalSearch::addNodeOperator(NodeOperator &op)
{
    nodeOps.emplace_back(&op);
}

void LocalSearch::addRouteOperator(RouteOperator &op)
{
    routeOps.emplace_back(&op);
}

std::vector<NodeOperator *> const &LocalSearch::nodeOperators() const
{
    return nodeOps;
}

std::vector<RouteOperator *> const &LocalSearch::routeOperators() const
{
    return routeOps;
}

void LocalSearch::setNeighbours(Neighbours neighbours)
{
    if (neighbours.size() != data.numLocations())
        throw std::runtime_error("Neighbourhood dimensions do not match.");

    for (size_t client = data.numDepots(); client != data.numLocations();
         ++client)
    {
        auto const beginPos = neighbours[client].begin();
        auto const endPos = neighbours[client].end();

        auto const pred = [&](auto item)
        { return item == client || item < data.numDepots(); };

        if (std::any_of(beginPos, endPos, pred))
        {
            throw std::runtime_error("Neighbourhood of client "
                                     + std::to_string(client)
                                     + " contains itself or a depot.");
        }
    }

    neighbours_ = neighbours;
}

LocalSearch::Neighbours const &LocalSearch::neighbours() const
{
    return neighbours_;
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

    std::for_each(nodeOps.begin(), nodeOps.end(), count);
    std::for_each(routeOps.begin(), routeOps.end(), count);

    assert(numImproving <= numUpdates_);
    return {numMoves, numImproving, numUpdates_};
}

LocalSearch::LocalSearch(ProblemData const &data, Neighbours neighbours)
    : data(data),
      neighbours_(data.numLocations()),
      orderNodes(data.numClients()),
      orderRoutes(data.numVehicles()),
      candidates(data.numLocations()),
      lastTestedRoutes(data.numVehicles()),
      lastUpdated(data.numVehicles())
{
    setNeighbours(neighbours);

    std::iota(orderNodes.begin(), orderNodes.end(), data.numDepots());
    std::iota(orderRoutes.begin(), orderRoutes.end(), 0);

    size_t offset = 0;
    for (size_t vehType = 0; vehType != data.numVehicleTypes(); vehType++)
    {
        orderVehTypes.emplace_back(vehType, offset);
        offset += data.vehicleType(vehType).numAvailable;
    }

    nodes.reserve(data.numLocations());
    for (size_t loc = 0; loc != data.numLocations(); ++loc)
        nodes.emplace_back(loc);

    routes.reserve(data.numVehicles());
    size_t rIdx = 0;
    for (size_t vehType = 0; vehType != data.numVehicleTypes(); ++vehType)
    {
        auto const numAvailable = data.vehicleType(vehType).numAvailable;
        for (size_t vehicle = 0; vehicle != numAvailable; ++vehicle)
            routes.emplace_back(data, rIdx++, vehType);
    }
}
