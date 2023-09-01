#include "LocalSearch.h"
#include "Measure.h"
#include "TimeWindowSegment.h"
#include "primitives.h"

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

        // Node operators are evaluated for neighbouring (U, V) pairs.
        for (auto const uClient : orderNodes)
        {
            auto *U = &clients[uClient];

            auto const lastTestedNode = lastTestedNodes[uClient];
            lastTestedNodes[uClient] = numMoves;

            // First test removing or inserting U. Particularly relevant if not
            // all clients are required (e.g., when prize collecting).
            applyOptionalClientMoves(U, costEvaluator);

            if (!U->route())  // we already evaluated inserting U, so there is
                continue;     // nothing left to be done for this client.

            // We next apply the regular node operators. These work on pairs
            // of nodes (U, V), where both U and V are in the solution.
            for (auto const vClient : neighbours[uClient])
            {
                auto *V = &clients[vClient];

                if (!V->route())
                    continue;

                if (lastModified[U->route()->idx()] > lastTestedNode
                    || lastModified[V->route()->idx()] > lastTestedNode)
                {
                    if (applyNodeOps(U, V, costEvaluator))
                        continue;

                    if (p(V)->isDepot() && applyNodeOps(U, p(V), costEvaluator))
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

            auto const lastTested = lastTestedRoutes[U.idx()];
            lastTestedRoutes[U.idx()] = numMoves;

            for (size_t rV = 0; rV != U.idx(); ++rV)
            {
                auto &V = routes[rV];

                if (V.empty() || !U.overlapsWith(V, overlapTolerance))
                    continue;

                auto const lastModifiedRoute
                    = std::max(lastModified[U.idx()], lastModified[V.idx()]);

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
            auto *routeU = U->route();  // copy these because the operator can
            auto *routeV = V->route();  // modify the node's route membership

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

            return true;
        }

    return false;
}

void LocalSearch::applyEmptyRouteMoves(Route::Node *U,
                                       CostEvaluator const &costEvaluator)
{
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

        if (U->route())  // try inserting U into the empty route.
            applyNodeOps(U, (*empty)[0], costEvaluator);
    }
}

void LocalSearch::applyOptionalClientMoves(Route::Node *U,
                                           CostEvaluator const &costEvaluator)
{
    auto const uClient = U->client();
    auto const &uData = data.client(uClient);

    // First test removing U. This is allowed when U is not required.
    if (!uData.required && removeCost(U, data, costEvaluator) < 0)
    {
        auto *route = U->route();
        route->remove(U->idx());
        update(route, route);
    }

    // If U is not currently in the solution, we test if inserting it is an
    // improving move. Note that we always insert required clients.
    if (!U->route())
    {
        // We take this as a default value in case none of the client's
        // neighbours are assigned, yet U is required.
        Route::Node *UAfter = routes[0][0];
        Cost bestCost = insertCost(U, UAfter, data, costEvaluator);

        for (auto const vClient : neighbours[uClient])
        {
            auto *V = &clients[vClient];

            if (!V->route())
                continue;

            auto cost = insertCost(U, V, data, costEvaluator);
            if (cost < bestCost)
            {
                bestCost = cost;
                UAfter = V;
            }
        }

        if (uData.required || bestCost < 0)
        {
            UAfter->route()->insert(UAfter->idx() + 1, U);
            update(UAfter->route(), UAfter->route());
        }
    }
}

void LocalSearch::update(Route *U, Route *V)
{
    numMoves++;
    searchCompleted = false;

    U->update();
    lastModified[U->idx()] = numMoves;

    for (auto *op : routeOps)  // this is used by some route operators
        op->update(U);         // to keep caches in sync.

    if (U != V)
    {
        V->update();
        lastModified[V->idx()] = numMoves;

        for (auto *op : routeOps)  // this is used by some route operators
            op->update(V);         // to keep caches in sync.
    }
}

void LocalSearch::loadSolution(Solution const &solution)
{
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

        route.update();
    }

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

        std::vector<size_t> visits;
        visits.reserve(route.size());

        for (auto *node : route)
            visits.push_back(node->client());

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
    for (size_t vehType = 0; vehType != data.numVehicleTypes(); ++vehType)
    {
        auto const numAvailable = data.vehicleType(vehType).numAvailable;
        for (size_t vehicle = 0; vehicle != numAvailable; ++vehicle)
            routes.emplace_back(data, rIdx++, vehType);
    }
}
