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
                                   CostEvaluator const &costEvaluator)
{
    loadSolution(solution);
    destroy();
    repair(costEvaluator);
    return exportSolution();
}

void DestroyRepair::destroy()
{
    auto const maxDestroy = std::min(40UL, data.numClients());
    size_t numDestroy = rng.randint(maxDestroy);
    numDestroy = std::max(10UL, numDestroy);

    for (size_t idx = 0; idx < numDestroy; ++idx)
    {
        auto const client = rng.randint(data.numClients()) + data.numDepots();
        auto *U = &nodes[client];

        if (!U->route())
            continue;

        auto *route = U->route();
        route->remove(U->idx());
        route->update();
    }
}

void DestroyRepair::repair(CostEvaluator const &costEvaluator)
{
    for (size_t client = data.numDepots(); client != data.numLocations();
         ++client)
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
    : data(data), rng(rng), neighbours_(data.numLocations())
{
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
