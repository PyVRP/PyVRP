#include "LocalSearch.h"
#include "Measure.h"
#include "TimeWindowSegment.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <stdexcept>
#include <vector>

using pyvrp::Solution;
using pyvrp::search::LocalSearch;
using TWS = pyvrp::TimeWindowSegment;

Solution LocalSearch::operator()(Solution const &solution,
                                 CostEvaluator const &costEvaluator)
{
    loadSolution(solution);

    while (true)
    {
        search(costEvaluator);

        // When numMoves != 0, search() modified the currently loaded solution.
        // In that case we need to reload the solution because intensify()'s
        // route operators need to update their caches.
        // TODO remove this.
        if (numMoves != 0)
        {
            Solution const newSol = exportSolution();
            loadSolution(newSol);
        }

        intensify(costEvaluator);

        if (numMoves == 0)  // then the current solution is locally optimal.
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
                                CostEvaluator const &costEvaluator,
                                double overlapTolerance)
{
    loadSolution(solution);
    intensify(costEvaluator, overlapTolerance);
    return exportSolution();
}

void LocalSearch::search(CostEvaluator const &costEvaluator)
{
    if (nodeOps.empty())
        return;

    // Caches the last time nodes were tested for modification (uses numMoves to
    // track this). The lastModified field, in contrast, track when a route was
    // last *actually* modified.
    std::vector<int> lastTestedNodes(data.numClients() + 1, -1);
    lastModified = std::vector<int>(data.numVehicles(), 0);

    searchCompleted = false;
    numMoves = 0;

    for (int step = 0; !searchCompleted; ++step)
    {
        searchCompleted = true;

        // Node operators are evaluated at neighbouring (U, V) pairs.
        for (auto const uClient : orderNodes)
        {
            auto *U = &clients[uClient];

            auto const lastTestedNode = lastTestedNodes[uClient];
            lastTestedNodes[uClient] = numMoves;

            if (U->route && !data.client(uClient).required)  // test removing U
                maybeRemove(U, costEvaluator);

            for (auto const vClient : neighbours[uClient])
            {
                auto *V = &clients[vClient];

                if (!U->route && V->route)             // U might be inserted
                    maybeInsert(U, V, costEvaluator);  // into V's route

                if (!U->route || !V->route)  // we already tested inserting U,
                    continue;                // so we can skip this move

                if (lastModified[U->route->idx] > lastTestedNode
                    || lastModified[V->route->idx] > lastTestedNode)
                {
                    if (applyNodeOps(U, V, costEvaluator))
                        continue;

                    if (p(V)->isDepot() && applyNodeOps(U, p(V), costEvaluator))
                        continue;
                }
            }

            if (step > 0)  // empty moves are not tested initially to avoid
            {              // using too many routes.
                auto begin = routes.begin();
                for (size_t t = 0; t != data.numVehicleTypes(); t++)
                {
                    // Check move involving empty route of each vehicle type
                    // (if such a route exists).
                    auto const end = begin + data.vehicleType(t).numAvailable;
                    auto pred = [](auto const &route) { return route.empty(); };
                    auto empty = std::find_if(begin, end, pred);
                    begin = end;

                    if (empty == end)
                        continue;

                    if (U->route)  // try inserting U into the empty route.
                        applyNodeOps(U, &empty->startDepot, costEvaluator);
                    else  // U is not in the solution, so again try inserting.
                        maybeInsert(U, &empty->startDepot, costEvaluator);
                }
            }
        }
    }
}

void LocalSearch::intensify(CostEvaluator const &costEvaluator,
                            double overlapTolerance)
{
    if (overlapTolerance < 0 || overlapTolerance > 1)
        throw std::runtime_error("overlapTolerance must be in [0, 1].");

    if (routeOps.empty())
        return;

    std::vector<int> lastTestedRoutes(data.numVehicles(), -1);
    lastModified = std::vector<int>(data.numVehicles(), 0);

    searchCompleted = false;
    numMoves = 0;

    while (!searchCompleted)
    {
        searchCompleted = true;

        for (int const rU : orderRoutes)
        {
            auto &U = routes[rU];

            if (U.empty())
                continue;

            auto const lastTested = lastTestedRoutes[U.idx];
            lastTestedRoutes[U.idx] = numMoves;

            for (size_t rV = 0; rV != U.idx; ++rV)
            {
                auto &V = routes[rV];

                if (V.empty() || !U.overlapsWith(V, overlapTolerance))
                    continue;

                auto const lastModifiedRoute
                    = std::max(lastModified[U.idx], lastModified[V.idx]);

                if (lastModifiedRoute > lastTested
                    && applyRouteOps(&U, &V, costEvaluator))
                    continue;
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
}

bool LocalSearch::applyNodeOps(Route::Node *U,
                               Route::Node *V,
                               CostEvaluator const &costEvaluator)
{
    for (auto *nodeOp : nodeOps)
        if (nodeOp->evaluate(U, V, costEvaluator) < 0)
        {
            auto *routeU = U->route;  // copy pointers because the operator can
            auto *routeV = V->route;  // modify the node's route membership

            nodeOp->apply(U, V);
            update(routeU, routeV);

            return true;
        }

    return false;
}

bool LocalSearch::applyRouteOps(Route *U,
                                Route *V,
                                CostEvaluator const &costEvaluator)
{
    for (auto *routeOp : routeOps)
        if (routeOp->evaluate(U, V, costEvaluator) < 0)
        {
            routeOp->apply(U, V);
            update(U, V);

            for (auto *op : routeOps)  // this is used by some route operators
            {                          // (particularly SWAP*) to keep caches
                op->update(U);         // in sync.
                op->update(V);
            }

            return true;
        }

    return false;
}

void LocalSearch::maybeInsert(Route::Node *U,
                              Route::Node *V,
                              CostEvaluator const &costEvaluator)
{
    assert(!U->route && V->route);

    Distance const deltaDist = data.dist(V->client, U->client)
                               + data.dist(U->client, n(V)->client)
                               - data.dist(V->client, n(V)->client);

    auto const &uClient = data.client(U->client);
    Cost deltaCost = static_cast<Cost>(deltaDist) - uClient.prize;

    deltaCost += costEvaluator.loadPenalty(V->route->load() + uClient.demand,
                                           V->route->capacity());
    deltaCost
        -= costEvaluator.loadPenalty(V->route->load(), V->route->capacity());

    // If this is true, adding U cannot decrease time warp in V's route enough
    // to offset the deltaCost.
    if (deltaCost >= costEvaluator.twPenalty(V->route->timeWarp()))
        return;

    auto const vTWS
        = TWS::merge(data.durationMatrix(), V->twBefore, U->tw, n(V)->twAfter);

    deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(V->route->timeWarp());

    if (deltaCost < 0)
    {
        V->route->insert(V->position + 1, U);
        update(V->route, V->route);
    }
}

void LocalSearch::maybeRemove(Route::Node *U,
                              CostEvaluator const &costEvaluator)
{
    assert(U->route);

    Distance const deltaDist = data.dist(p(U)->client, n(U)->client)
                               - data.dist(p(U)->client, U->client)
                               - data.dist(U->client, n(U)->client);

    auto const &uClient = data.client(U->client);
    Cost deltaCost = static_cast<Cost>(deltaDist) + uClient.prize;

    deltaCost += costEvaluator.loadPenalty(U->route->load() - uClient.demand,
                                           U->route->capacity());
    deltaCost
        -= costEvaluator.loadPenalty(U->route->load(), U->route->capacity());

    auto uTWS
        = TWS::merge(data.durationMatrix(), p(U)->twBefore, n(U)->twAfter);

    deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());

    if (deltaCost < 0)
    {
        auto *route = U->route;  // after remove(), U->route is a nullptr
        route->remove(U->position);
        update(route, route);
    }
}

void LocalSearch::update(Route *U, Route *V)
{
    numMoves++;
    searchCompleted = false;

    U->update();
    lastModified[U->idx] = numMoves;

    if (U != V)
    {
        V->update();
        lastModified[V->idx] = numMoves;
    }
}

void LocalSearch::loadSolution(Solution const &solution)
{
    if (!solution.isComplete())  // TODO allow incomplete at some point
        throw std::runtime_error("LocalSearch requires complete solutions.");

    for (size_t client = 0; client <= data.numClients(); client++)
    {
        clients[client].client = client;
        clients[client].tw = {client, data.client(client)};
        clients[client].route = nullptr;  // nullptr implies "not in solution"
    }

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
    for (auto const &solRoute : solution.getRoutes())
    {
        // Determine index of next route of this type to load, where we rely
        // on solution to be valid to not exceed the number of vehicles per
        // vehicle type.
        auto const r = vehicleOffset[solRoute.vehicleType()]++;
        Route &route = routes[r];

        assert(route.empty());  // should have been emptied above.

        for (auto const client : solRoute)
            route.push_back(&clients[client]);
    }

    for (auto &route : routes)
        route.update();

    for (auto *routeOp : routeOps)
        routeOp->init(solution);
}

Solution LocalSearch::exportSolution() const
{
    std::vector<Solution::Route> solRoutes;
    solRoutes.reserve(data.numVehicles());

    for (auto const &route : routes)
    {
        if (route.empty())
            continue;

        std::vector<int> visits;
        visits.reserve(route.size());

        for (auto *node : route)
            visits.push_back(node->client);

        solRoutes.emplace_back(data, visits, route.vehicleType());
    }

    return {data, solRoutes};
}

void LocalSearch::addNodeOperator(NodeOp &op) { nodeOps.emplace_back(&op); }

void LocalSearch::addRouteOperator(RouteOp &op) { routeOps.emplace_back(&op); }

void LocalSearch::setNeighbours(Neighbours neighbours)
{
    if (neighbours.size() != data.numClients() + 1)
        throw std::runtime_error("Neighbourhood dimensions do not match.");

    for (size_t client = 0; client <= data.numClients(); ++client)
    {
        auto const beginPos = neighbours[client].begin();
        auto const endPos = neighbours[client].end();

        auto const clientPos = std::find(beginPos, endPos, client);
        auto const depotPos = std::find(beginPos, endPos, 0);

        if (clientPos != endPos || depotPos != endPos)
        {
            throw std::runtime_error("Neighbourhood of client "
                                     + std::to_string(client)
                                     + " contains itself or the depot.");
        }
    }

    this->neighbours = neighbours;
}

LocalSearch::Neighbours const &LocalSearch::getNeighbours() const
{
    return neighbours;
}

LocalSearch::LocalSearch(ProblemData const &data, Neighbours neighbours)
    : data(data),
      neighbours(data.numClients() + 1),
      orderNodes(data.numClients()),
      orderRoutes(data.numVehicles()),
      lastModified(data.numVehicles(), -1)
{
    setNeighbours(neighbours);

    std::iota(orderNodes.begin(), orderNodes.end(), 1);
    std::iota(orderRoutes.begin(), orderRoutes.end(), 0);

    clients.reserve(data.numClients() + 1);
    for (size_t client = 0; client <= data.numClients(); ++client)
        clients.emplace_back(client);

    routes.reserve(data.numVehicles());
    size_t rIdx = 0;
    for (size_t vehTypeIdx = 0; vehTypeIdx != data.numVehicleTypes();
         ++vehTypeIdx)
    {
        auto const &vehType = data.vehicleType(vehTypeIdx);
        auto const numAvailable = vehType.numAvailable;

        for (size_t i = 0; i != numAvailable; ++i)
        {
            routes.emplace_back(data, rIdx, vehTypeIdx);
            rIdx++;
        }
    }
}
