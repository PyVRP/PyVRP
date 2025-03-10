#include "DestroyRepair.h"
#include "Measure.h"
#include "primitives.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numeric>

using pyvrp::Solution;
using pyvrp::search::DestroyRepair;

Solution DestroyRepair::operator()(Solution const &solution,
                                   CostEvaluator const &costEvaluator,
                                   size_t numDestroy)
{
    loadSolution(solution);
    destroy(numDestroy);
    repair(costEvaluator);
    return exportSolution();
}

void DestroyRepair::destroy(size_t numDestroy)
{
    std::shuffle(orderNodes.begin(), orderNodes.end(), rng);
    auto const dIdx = rng.randint(2);

    if (dIdx == 0)
        concentric(numDestroy);
    else
        strings(numDestroy);
}

void DestroyRepair::concentric(size_t numDestroy)
{
    auto const center = rng.randint(data.numClients()) + data.numDepots();
    auto const &neighbours = neighbours_[center];
    auto const maxDestroy = std::min(numDestroy, neighbours.size());

    size_t numDestroyed = 0;
    for (auto neighbour : neighbours)  // assumes there are enough neighbours
    {
        auto *U = &nodes[neighbour];
        if (!U->route())
            continue;

        auto *route = U->route();
        route->remove(U->idx());
        route->update();

        if (++numDestroyed == maxDestroy)
            return;
    }
}

void DestroyRepair::strings(size_t numDestroy)
{
    auto const center = rng.randint(data.numClients()) + data.numDepots();
    auto const &neighbours = neighbours_[center];
    auto const maxDestroy = std::min(numDestroy, neighbours.size());
    auto const maxPerRoute = 5;

    size_t numDestroyed = 0;
    for (auto const neighbour : neighbours)
    {
        auto *U = &nodes[neighbour];
        if (!U->route())
            continue;

        auto *route = U->route();
        size_t routeDestroyed = 0;  // Count per route

        while (!n(U)->isDepot() && routeDestroyed < maxPerRoute)
        {
            auto next = n(U);
            route->remove(U->idx());
            route->update();
            U = next;

            routeDestroyed++;
            if (++numDestroyed == numDestroy)
                return;
        }
    }
}

void DestroyRepair::repair(CostEvaluator const &costEvaluator)
{
    std::shuffle(orderNodes.begin(), orderNodes.end(), rng);

    auto const dIdx = rng.randint(2);

    if (dIdx == 0)
        greedyInsert(costEvaluator);
    else
    {
        // HACK this allows for infeasible insertions
        auto const costEval = CostEvaluator({20}, 6, 6);
        greedyInsert(costEval);
    }
}

void DestroyRepair::greedyInsert(CostEvaluator const &costEvaluator)
{
    for (auto const client : orderNodes)
    {
        auto *U = &nodes[client];
        if (U->route())
            continue;

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

        auto begin = routes.begin();
        for (size_t vehType = 0; vehType != data.numVehicleTypes(); vehType++)
        {
            auto const end = begin + data.vehicleType(vehType).numAvailable;
            auto const pred = [](auto const &route) { return route.empty(); };
            auto empty = std::find_if(begin, end, pred);
            begin = end;

            if (empty != end)  // try inserting U into the empty route.
            {
                auto const cost
                    = insertCost(U, (*empty)[0], data, costEvaluator);
                if (cost < bestCost)
                {
                    bestCost = cost;
                    UAfter = (*empty)[0];
                }
            }
        }

        assert(UAfter && UAfter->route());
        UAfter->route()->insert(UAfter->idx() + 1, U);
        UAfter->route()->update();
    }
}

void DestroyRepair::update(Route *U, Route *V)
{
    U->update();

    if (U != V)
        V->update();
}

void DestroyRepair::loadSolution(Solution const &solution)
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
    for (auto const &solRoute : solution.routes())
    {
        // Set up a container of all node visits. This lets us insert all
        // nodes in one go, requiring no intermediate updates.
        std::vector<Route::Node *> visits;
        visits.reserve(solRoute.size());
        for (auto const client : solRoute)
            visits.push_back(&nodes[client]);

        // Determine index of next route of this type to load, where we rely
        // on solution to be valid to not exceed the number of vehicles per
        // vehicle type.
        auto const idx = vehicleOffset[solRoute.vehicleType()]++;
        routes[idx].insert(1, visits.begin(), visits.end());
        routes[idx].update();
    }
}

Solution DestroyRepair::exportSolution() const
{
    std::vector<pyvrp::Route> solRoutes;
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

void DestroyRepair::setNeighbours(Neighbours neighbours)
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

DestroyRepair::Neighbours const &DestroyRepair::neighbours() const
{
    return neighbours_;
}

DestroyRepair::DestroyRepair(ProblemData const &data,
                             RandomNumberGenerator &rng,
                             Neighbours neighbours)
    : data(data),
      rng(rng),
      neighbours_(data.numLocations()),
      orderNodes(data.numClients())

{
    std::iota(orderNodes.begin(), orderNodes.end(), data.numDepots());

    setNeighbours(neighbours);
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
