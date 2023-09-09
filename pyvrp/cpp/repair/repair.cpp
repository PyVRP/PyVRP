#include "repair.h"

#include <cassert>

using pyvrp::Solution;
using pyvrp::search::Route;

using Clients = std::vector<Route::Node>;
using Routes = std::vector<Route>;
using SolRoutes = std::vector<Solution::Route>;

void pyvrp::repair::setupRoutes(Clients &clients,
                                Routes &routes,
                                SolRoutes const &solRoutes,
                                ProblemData const &data)
{
    assert(clients.empty() && routes.empty());

    // Doing this avoids re-allocations, which would break the pointer structure
    // that Route and Route::Node use.
    clients.reserve(data.numClients() + 1);
    routes.reserve(solRoutes.size());

    for (size_t client = 0; client <= data.numClients(); ++client)
        clients.emplace_back(client);

    size_t idx = 0;
    for (auto const &solRoute : solRoutes)
    {
        auto &route = routes.emplace_back(data, idx++, solRoute.vehicleType());
        for (auto const client : solRoute)
            route.push_back(&clients[client]);

        route.update();
    }
}

Solution pyvrp::repair::exportRoutes(ProblemData const &data,
                                     Routes const &routes)
{
    std::vector<Solution::Route> solRoutes;
    solRoutes.reserve(routes.size());

    for (auto const &route : routes)
    {
        std::vector<size_t> visits;
        visits.reserve(route.size());

        for (auto *node : route)
            visits.push_back(node->client());

        solRoutes.emplace_back(data, visits, route.vehicleType());
    }

    return {data, solRoutes};
}
