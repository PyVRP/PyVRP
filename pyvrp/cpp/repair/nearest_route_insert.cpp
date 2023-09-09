#include "nearest_route_insert.h"
#include "repair.h"

#include "TimeWindowSegment.h"
#include "search/primitives.h"

#include <cassert>
#include <cmath>
#include <limits>

using pyvrp::Solution;
using pyvrp::search::insertCost;
using pyvrp::search::Route;

using Clients = std::vector<Route::Node>;
using Routes = std::vector<Route>;
using SolRoutes = std::vector<Solution::Route>;

Solution pyvrp::repair::nearestRouteInsert(SolRoutes const &solRoutes,
                                           std::vector<size_t> const &unplanned,
                                           ProblemData const &data,
                                           CostEvaluator const &costEvaluator)
{
    if (solRoutes.empty() && !unplanned.empty())
        throw std::invalid_argument("Need routes to repair!");

    Clients clients;
    Routes routes;
    setupRoutes(clients, routes, solRoutes, data);

    for (size_t client : unplanned)
    {
        Route::Node *U = &clients[client];
        assert(!U->route());

        auto const x = static_cast<double>(data.client(client).x);
        auto const y = static_cast<double>(data.client(client).y);

        // Determine non-empty route with centroid nearest to this client.
        auto bestDistance = std::numeric_limits<double>::max();
        auto bestRouteIdx = 0;
        for (size_t rIdx = 0; rIdx != routes.size(); ++rIdx)
        {
            auto const &route = routes[rIdx];

            if (route.empty())
                continue;

            auto const distance = std::hypot(x - route.centroid().first,
                                             y - route.centroid().second);

            if (distance < bestDistance)
            {
                bestRouteIdx = rIdx;
                bestDistance = distance;
            }
        }

        // Find best insertion point in selected route.
        auto &route = routes[bestRouteIdx];
        Cost bestCost = insertCost(U, route[0], data, costEvaluator);
        auto offset = 0;

        for (auto *V : route)  // evaluate after V
        {
            auto const cost = insertCost(U, V, data, costEvaluator);
            if (cost < bestCost)
            {
                bestCost = cost;
                offset = V->idx() + 1;
            }
        }

        route.insert(offset, U);
        route.update();
    }

    return exportRoutes(data, routes);
}
