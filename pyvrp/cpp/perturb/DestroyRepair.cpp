#include "DestroyRepair.h"
#include "RandomNumberGenerator.h"
#include "Trip.h"
#include "search/Route.h"

#include <cassert>

using pyvrp::Solution;
using pyvrp::perturb::DestroyRepair;

Solution
DestroyRepair::operator()(Solution const &solution,
                          CostEvaluator const &costEvaluator,
                          std::vector<std::vector<size_t>> const &neighbours,
                          RandomNumberGenerator &rng)
{
    loadSolution(solution);

    if (!destroyOps.empty())
    {
        auto *op = destroyOps[rng.randint(destroyOps.size())];
        (*op)(nodes, routes, costEvaluator, neighbours, rng);
    }

    if (!repairOps.empty())
    {
        auto *op = repairOps[rng.randint(repairOps.size())];
        (*op)(nodes, routes, costEvaluator, neighbours, rng);
    }

    return exportSolution();
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
                pyvrp::search::Route::Node depot = {trip.startDepot()};
                route.push_back(&depot);
            }

            for (auto const client : trip)
                route.push_back(&nodes[client]);
        }

        route.update();
    }
}

Solution DestroyRepair::exportSolution() const
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

    return {data, solRoutes};
}

void DestroyRepair::addDestroyOperator(DestroyOperator &op)
{
    destroyOps.emplace_back(&op);
}

void DestroyRepair::addRepairOperator(RepairOperator &op)
{
    repairOps.emplace_back(&op);
}

DestroyRepair::DestroyRepair(ProblemData const &data) : data(data)
{
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
