#include "LocalSearch.h"
#include "DynamicBitset.h"
#include "Measure.h"
#include "Trip.h"
#include "primitives.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <numeric>

using pyvrp::Solution;
using pyvrp::search::LocalSearch;
using pyvrp::search::NodeOperator;
using pyvrp::search::RouteOperator;
using pyvrp::search::SearchSpace;

Solution LocalSearch::operator()(Solution const &solution,
                                 CostEvaluator const &costEvaluator)
{
    loadSolution(solution);
    perturb(costEvaluator);

    while (true)
    {
        search(costEvaluator);
        auto const numUpdates = numUpdates_;  // after node search

        intensify(costEvaluator);
        if (numUpdates_ == numUpdates)
            // Then intensify (route search) did not do any additional
            // updates, so the solution is locally optimal.
            break;
    }

    return exportSolution();
}

Solution LocalSearch::search(Solution const &solution,
                             CostEvaluator const &costEvaluator)
{
    loadSolution(solution);
    search(costEvaluator);
    return exportSolution();
}

Solution LocalSearch::intensify(Solution const &solution,
                                CostEvaluator const &costEvaluator)
{
    loadSolution(solution);
    intensify(costEvaluator);
    return exportSolution();
}

Solution LocalSearch::perturb(Solution const &solution,
                              CostEvaluator const &costEvaluator)
{
    loadSolution(solution);
    perturb(costEvaluator);
    return exportSolution();
}

void LocalSearch::search(CostEvaluator const &costEvaluator)
{
    if (nodeOps.empty())
        return;

    searchCompleted_ = false;
    for (int step = 0; !searchCompleted_; ++step)
    {
        searchCompleted_ = true;

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

            if (!U->route())  // we already evaluated inserting U, so there is
                continue;     // nothing left to be done for this client.

            // If U borders a reload depot, try removing it.
            applyDepotRemovalMove(p(U), costEvaluator);
            applyDepotRemovalMove(n(U), costEvaluator);

            // We next apply the regular operators that work on pairs of nodes
            // (U, V), where both U and V are in the solution. We only do this
            // if U is a promising candidate for improvement.
            if (!searchSpace_.isPromising(uClient))
                continue;

            for (auto const vClient : searchSpace_.neighboursOf(uClient))
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
            if (step > 0)
                applyEmptyRouteMoves(U, costEvaluator);
        }
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

void LocalSearch::perturb(CostEvaluator const &costEvaluator)
{
    size_t movesLeft = perturbationManager_.numPerturbations();

    if (!movesLeft)  // nothing to do
        return;

    enum class PerturbType
    {
        REMOVE,
        INSERT
    };

    // Clear the set of promising nodes. Perturbation determines the initial
    // set of promising nodes for further (local search) improvement.
    searchSpace_.unmarkAllPromising();

    DynamicBitset perturbed = {data.numLocations()};
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
            markPromising(node);
            route->remove(node->idx());
            route->update();
        }
        // Insert if node is not in a route and we are currently inserting.
        else if (!route && action == PerturbType::INSERT)
            insert(node, costEvaluator, true);
        else  // no-op
            return;

        perturbed[node->client()] = true;
        movesLeft--;
    };

    // We do numPerturbations if we can. We perturb the local neighbourhood of
    // randomly selected clients U: if U is in the solution, we remove it and
    // its neighbours, while if it is not, we try to insert instead. Each
    // removal or insertion counts as one perturbation.
    for (auto const uClient : orderNodes)
    {
        auto *U = &nodes[uClient];
        auto action = U->route() ? PerturbType::REMOVE : PerturbType::INSERT;
        perturb(U, action);

        if (!movesLeft)
            return;

        for (auto const vClient : searchSpace_.neighboursOf(uClient))
        {
            auto *V = &nodes[vClient];
            perturb(V, action);

            if (!movesLeft)
                return;
        }
    }
}

void LocalSearch::shuffle(RandomNumberGenerator &rng)
{
    perturbationManager_.shuffle(rng);

    rng.shuffle(orderNodes.begin(), orderNodes.end());
    rng.shuffle(orderRoutes.begin(), orderRoutes.end());
    rng.shuffle(orderVehTypes.begin(), orderVehTypes.end());

    rng.shuffle(nodeOps.begin(), nodeOps.end());
    rng.shuffle(routeOps.begin(), routeOps.end());
}

bool LocalSearch::applyNodeOps(Route::Node *U,
                               Route::Node *V,
                               CostEvaluator const &costEvaluator)
{
    for (auto *nodeOp : nodeOps)
    {
        auto const deltaCost = nodeOp->evaluate(U, V, costEvaluator);
        if (deltaCost < 0)
        {
            auto *rU = U->route();  // copy these because the operator can
            auto *rV = V->route();  // modify the nodes' route membership

            [[maybe_unused]] auto const costBefore
                = costEvaluator.penalisedCost(*rU)
                  + Cost(rU != rV) * costEvaluator.penalisedCost(*rV);

            markPromising(U);
            markPromising(V);

            nodeOp->apply(U, V);
            update(rU, rV);

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
        markPromising(U);  // for U's neighbours, which might not be depots
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

    if (uData.required && !U->route())  // then we must insert U
        insert(U, costEvaluator, uData.required);

    // Required clients are not optional, and have just been inserted above
    // if not already in the solution. Groups have their own operator and are
    // not processed here.
    if (uData.required || uData.group)
        return;

    if (removeCost(U, data, costEvaluator) < 0)  // remove if improving
    {
        markPromising(U);
        auto *route = U->route();
        route->remove(U->idx());
        update(route, route);
    }

    if (U->route())
        return;

    // Attempt to re-insert U using a first-improving neighbourhood search.
    for (auto const vClient : searchSpace_.neighboursOf(U->client()))
    {
        auto *V = &nodes[vClient];
        auto *route = V->route();

        if (!route)
            continue;

        if (insertCost(U, V, data, costEvaluator) < 0)  // insert if improving
        {
            route->insert(V->idx() + 1, U);
            update(route, route);
            markPromising(U);
            return;
        }

        // We prefer inserting over replacing, but if V is not required and
        // replacing V with U is improving, we also do that now.
        ProblemData::Client const &vData = data.location(V->client());
        if (!vData.required && inplaceCost(U, V, data, costEvaluator) < 0)
        {
            markPromising(V);
            auto const idx = V->idx();
            route->remove(idx);
            route->insert(idx, U);
            update(route, route);
            markPromising(U);
            return;
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

        markPromising(&node);
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
        markPromising(U);
    }
}

void LocalSearch::insert(Route::Node *U,
                         CostEvaluator const &costEvaluator,
                         bool required)
{
    Route::Node *UAfter = routes[0][0];
    auto bestCost = insertCost(U, UAfter, data, costEvaluator);

    for (auto const vClient : searchSpace_.neighboursOf(U->client()))
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

    // Try inserting into the first found empty route. We do this in randomised
    // order of vehicle types to avoid over-prioritising vehicles with low fixed
    // costs (similar to ``applyEmptyRouteMoves``).
    for (auto const &[vehType, offset] : orderVehTypes)
    {
        auto const begin = routes.begin() + offset;
        auto const end = begin + data.vehicleType(vehType).numAvailable;
        auto const pred = [](auto const &route) { return route.empty(); };
        auto empty = std::find_if(begin, end, pred);

        if (empty == end)
            continue;

        auto const cost = insertCost(U, (*empty)[0], data, costEvaluator);
        if (cost < bestCost)
        {
            bestCost = cost;
            UAfter = (*empty)[0];
        }

        break;
    }

    if (required || bestCost < 0)
    {
        UAfter->route()->insert(UAfter->idx() + 1, U);
        update(UAfter->route(), UAfter->route());
        markPromising(U);
    }
}

void LocalSearch::markPromising(Route::Node const *U)
{
    assert(U->route());

    if (!U->isDepot())
        searchSpace_.markPromising(U->client());

    if (!U->isStartDepot() && !p(U)->isDepot())
        searchSpace_.markPromising(p(U)->client());

    if (!U->isEndDepot() && !n(U)->isDepot())
        searchSpace_.markPromising(n(U)->client());
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

void LocalSearch::loadSolution(Solution const &solution)
{
    std::fill(lastTestedNodes.begin(), lastTestedNodes.end(), -1);
    std::fill(lastTestedRoutes.begin(), lastTestedRoutes.end(), -1);
    std::fill(lastUpdated.begin(), lastUpdated.end(), 0);
    searchSpace_.markAllPromising();
    numUpdates_ = 0;

    // First empty all routes.
    for (auto &route : routes)
        route.clear();

    // Determine offsets for vehicle types.
    std::vector<size_t> vehicleOffset(data.numVehicleTypes(), 0);
    for (size_t vehType = 1; vehType < data.numVehicleTypes(); vehType++)
    {
        auto const prevAvail = data.vehicleType(vehType - 1).numAvailable;
        vehicleOffset[vehType] = vehicleOffset[vehType - 1] + prevAvail;
    }

    // Load routes from solution.
    for (auto const &solRoute : solution.routes())
    {
        // Determine index of next route of this type to load, where we rely
        // on solution to be valid to not exceed the number of vehicles per
        // vehicle type.
        auto const idx = vehicleOffset[solRoute.vehicleType()]++;
        auto &route = routes[idx];

        // Routes use a representation with nodes for each client, reload depot
        // (one per trip), and start/end depots. The start depot doubles as the
        // reload depot for the first trip.
        route.reserve(solRoute.size() + solRoute.numTrips() + 1);

        for (size_t tripIdx = 0; tripIdx != solRoute.numTrips(); ++tripIdx)
        {
            auto const &trip = solRoute.trip(tripIdx);

            if (tripIdx != 0)  // then we first insert a trip delimiter.
            {
                Route::Node depot = {trip.startDepot()};
                route.push_back(&depot);
            }

            for (auto const client : trip)
                route.push_back(&nodes[client]);
        }

        route.update();
    }

    for (auto *nodeOp : nodeOps)
        nodeOp->init(solution);

    for (auto *routeOp : routeOps)
        routeOp->init(solution);
}

Solution LocalSearch::exportSolution() const
{
    std::vector<pyvrp::Route> solRoutes;
    solRoutes.reserve(data.numVehicles());

    std::vector<Trip> trips;
    std::vector<size_t> visits;

    for (auto const &route : routes)
    {
        if (route.empty())
            continue;

        trips.clear();
        trips.reserve(route.numTrips());

        visits.clear();
        visits.reserve(route.numClients());

        auto const *prevDepot = route[0];
        for (size_t idx = 1; idx != route.size(); ++idx)
        {
            auto const *node = route[idx];

            if (!node->isDepot())
            {
                visits.push_back(node->client());
                continue;
            }

            trips.emplace_back(data,
                               visits,
                               route.vehicleType(),
                               prevDepot->client(),
                               node->client());

            visits.clear();
            prevDepot = node;
        }

        assert(trips.size() == route.numTrips());
        solRoutes.emplace_back(data, trips, route.vehicleType());
    }

    return {data, std::move(solRoutes)};
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

    std::for_each(nodeOps.begin(), nodeOps.end(), count);
    std::for_each(routeOps.begin(), routeOps.end(), count);

    assert(numImproving <= numUpdates_);
    return {numMoves, numImproving, numUpdates_};
}

LocalSearch::LocalSearch(ProblemData const &data,
                         SearchSpace::Neighbours neighbours,
                         PerturbationManager &perturbationManager)
    : data(data),
      searchSpace_(data, neighbours),
      perturbationManager_(perturbationManager),
      orderNodes(data.numClients()),
      orderRoutes(data.numVehicles()),
      lastTestedNodes(data.numLocations()),
      lastTestedRoutes(data.numVehicles()),
      lastUpdated(data.numVehicles())
{
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
