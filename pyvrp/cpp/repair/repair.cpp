#include "repair.h"

#include <cassert>

using pyvrp::Solution;

using SearchRoute = pyvrp::search::Route;
using SolRoute = pyvrp::Route;

void pyvrp::repair::setupRoutes(std::vector<SearchRoute::Node> &locs,
                                std::vector<SearchRoute> &routes,
                                std::vector<SolRoute> const &solRoutes,
                                ProblemData const &data)
{
    assert(locs.empty() && routes.empty());

    // Doing this avoids re-allocations, which would break the pointer structure
    // that Route and Route::Node use.
    locs.reserve(data.numLocations());
    routes.reserve(solRoutes.size());

    for (size_t loc = 0; loc != data.numLocations(); ++loc)
        locs.emplace_back(loc,
                          loc < data.numDepots()
                              ? SearchRoute::Node::NodeType::DepotLoad
                              : SearchRoute::Node::NodeType::Client);

    size_t idx = 0;
    for (auto const &solRoute : solRoutes)
    {
        auto const &vehicleType = data.vehicleType(solRoute.vehicleType());
        auto &route = routes.emplace_back(data, idx++, solRoute.vehicleType());

        for (size_t tripIdx = 0; tripIdx != solRoute.numTrips(); ++tripIdx)
        {
            if (tripIdx > 0)  // Create and insert depot nodes for new trip.
                route.emplace_back_depot(vehicleType.startDepot,
                                         vehicleType.endDepot);

            auto const &trip = solRoute.trip(tripIdx);
            for (auto const client : trip)
                route.push_back(&locs[client]);
        }

        route.update();
    }
}

std::vector<SolRoute>
pyvrp::repair::exportRoutes(ProblemData const &data,
                            std::vector<SearchRoute> const &routes)
{
    std::vector<SolRoute> solRoutes;
    solRoutes.reserve(routes.size());

    for (auto const &route : routes)
    {
        if (route.empty())
            continue;

        std::vector<std::vector<size_t>> trips;
        trips.reserve(route.numTrips());

        std::vector<size_t> trip;
        trip.reserve(route.size());  // upper bound
        for (auto *node : route)
        {
            if (node->type() == SearchRoute::Node::NodeType::DepotLoad)
                trip.clear();
            else if (node->type() == SearchRoute::Node::NodeType::Client)
                trip.push_back(node->client());
            else  // depot unload -> end of trip
                trips.push_back(trip);
        }

        solRoutes.emplace_back(data, trips, route.vehicleType());
    }

    return solRoutes;
}
