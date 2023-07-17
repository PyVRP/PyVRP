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

Solution LocalSearch::search(Solution &solution,
                             CostEvaluator const &costEvaluator)
{
    if (nodeOps.empty())
        return solution;

    loadSolution(solution);

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

            // Shuffling the neighbours in this loop should not matter much as
            // we are already randomizing the nodes U.
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

    return exportSolution();
}

Solution LocalSearch::intensify(Solution &solution,
                                CostEvaluator const &costEvaluator,
                                int overlapToleranceDegrees)
{
    if (routeOps.empty())
        return solution;

    loadSolution(solution);

    auto const overlapTolerance = overlapToleranceDegrees * 65536;

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

            // Shuffling in this loop should not matter much as we are
            // already randomizing the routes U.
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

    return exportSolution();
}

void LocalSearch::shuffle(XorShift128 &rng)
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
        U->insertAfter(V);           // U has no route, so there's nothing to
        update(V->route, V->route);  // update there.
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
        auto *route = U->route;  // after U->remove(), U->route is a nullptr
        U->remove();
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
    for (size_t client = 0; client <= data.numClients(); client++)
    {
        clients[client].tw = {static_cast<int>(client),  // TODO cast
                              static_cast<int>(client),  // TODO cast
                              data.client(client).serviceDuration,
                              0,
                              data.client(client).twEarly,
                              data.client(client).twLate,
                              data.client(client).releaseTime};

        clients[client].route = nullptr;  // nullptr implies "not in solution"
    }

    // First empty all routes
    for (auto &route : routes)
    {
        auto const &vehicleType = data.vehicleType(route.vehicleType());

        auto *startDepot = &route.startDepot;
        auto *endDepot = &route.endDepot;

        startDepot->prev = endDepot;
        startDepot->next = endDepot;

        endDepot->prev = startDepot;
        endDepot->next = startDepot;

        startDepot->tw = clients[vehicleType.depot].tw;
        startDepot->twBefore = clients[vehicleType.depot].tw;

        endDepot->tw = clients[vehicleType.depot].tw;
        endDepot->twAfter = clients[vehicleType.depot].tw;
    }

    // Determine offsets for vehicle types.
    std::vector<size_t> vehicleOffset(data.numVehicleTypes(), 0);
    for (size_t vehType = 1; vehType < data.numVehicleTypes(); vehType++)
    {
        auto const available = data.vehicleType(vehType).numAvailable;
        vehicleOffset[vehType] = vehicleOffset[vehType - 1] + available;
    }

    // Load routes from solution.
    for (auto const &solRoute : solution.getRoutes())
    {
        // Determine index of next route of this type to load, where we rely
        // on solution to be valid to not exceed the number of vehicles per
        // vehicle type.
        auto const r = vehicleOffset[solRoute.vehicleType()]++;
        Route *route = &routes[r];

        auto *client = &clients[solRoute[0]];
        client->route = route;

        client->prev = &route->startDepot;
        route->startDepot.next = client;

        for (size_t idx = 1; idx < solRoute.size(); idx++)
        {
            auto *prev = client;

            client = &clients[solRoute[idx]];
            client->route = route;

            client->prev = prev;
            prev->next = client;
        }

        client->next = &route->endDepot;
        route->endDepot.prev = client;
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

    auto isEmpty = [](auto const &neighbours) { return neighbours.empty(); };
    if (std::all_of(neighbours.begin(), neighbours.end(), isEmpty))
        throw std::runtime_error("Neighbourhood is empty.");

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
