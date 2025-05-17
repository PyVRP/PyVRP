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
    loadSolution(solution, data, routes, nodes);
    destroy(numDestroy);
    repair(costEvaluator);
    return exportSolution(routes, data);
}

void DestroyRepair::destroy(size_t numDestroy)
{
    std::shuffle(orderNodes.begin(), orderNodes.end(), rng);
    auto const dIdx = rng.randint(2);

    if (rng.randint(2))
        swaproutes(numDestroy);  // rename to change route type upgrade route

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
    auto const maxPerRoute = 8;

    size_t numDestroyed = 0;
    for (auto const neighbour : neighbours)
    {
        auto *U = &nodes[neighbour];
        if (!U->route())
            continue;

        auto *route = U->route();
        size_t routeDestroyed = 0;  // Count per route

        while (!U->isDepot() && routeDestroyed < maxPerRoute)
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

void DestroyRepair::swaproutes(size_t numDestroy)
{
    auto const idxU = rng.randint(data.numClients()) + data.numDepots();
    auto *rU = nodes[idxU].route();
    if (!rU)  // no route so skip
        return;

    // Find the indices of empty routes per vehicle type.
    std::vector<size_t> emptyRouteIdcs;
    auto begin = routes.begin();

    for (size_t vehType = 0; vehType != data.numVehicleTypes(); vehType++)
    {
        auto const end = begin + data.vehicleType(vehType).numAvailable;
        auto const pred = [](auto const &route) { return route.empty(); };
        auto empty = std::find_if(begin, end, pred);

        if (empty != end && rU->vehicleType() != vehType)
            emptyRouteIdcs.push_back(std::distance(routes.begin(), empty));

        begin = end;
    }

    if (!emptyRouteIdcs.empty())
    {
        // Change the vehicle type of the route.
        std::shuffle(emptyRouteIdcs.begin(), emptyRouteIdcs.end(), rng);
        auto &empty = routes[emptyRouteIdcs[0]];
        op.apply((*rU)[0], empty[0]);
        rU->update();
        empty.update();
    }
}

void DestroyRepair::repair(CostEvaluator const &costEvaluator)
{
    std::shuffle(orderNodes.begin(), orderNodes.end(), rng);
    greedyInsert(costEvaluator);
}

void DestroyRepair::greedyInsert(CostEvaluator const &costEvaluator)
{
    for (auto const client : orderNodes)
    {
        auto *U = &nodes[client];
        if (U->route())
            continue;

        ProblemData::Client const &uData = data.location(client);

        // Skip optional clients most of the times. We don't always
        // skip/let this over to LS, because LS only inserts optional clients
        // greedily and this type of insertion changes the search space.
        if (!uData.required & (rng.randint(100) > 0))
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
                if (rng.randint(3) > 0)  // skip sometimes because too greedy
                    continue;            // results in low fixed costs vehicles

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

DestroyRepair::DestroyRepair(ProblemData const &data,
                             RandomNumberGenerator &rng,
                             Neighbours neighbours)
    : data(data),
      rng(rng),
      neighbours_(neighbours),
      orderNodes(data.numClients()),
      op(data)

{
    std::iota(orderNodes.begin(), orderNodes.end(), data.numDepots());

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
