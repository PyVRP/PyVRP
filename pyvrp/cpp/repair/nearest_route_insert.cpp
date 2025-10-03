#include "nearest_route_insert.h"
#include "repair.h"

#include "DurationSegment.h"
#include "search/primitives.h"

#include <algorithm>
#include <cassert>
#include <cmath>

using pyvrp::Solution;
using pyvrp::search::insertCost;

using SearchRoute = pyvrp::search::Route;
using SolRoute = pyvrp::Route;

std::vector<SolRoute>
pyvrp::repair::nearestRouteInsert(std::vector<SolRoute> const &solRoutes,
                                  std::vector<size_t> const &unplanned,
                                  ProblemData const &data,
                                  CostEvaluator const &costEvaluator)
{
    if (solRoutes.empty() && !unplanned.empty())
        throw std::invalid_argument("Need routes to repair!");

    std::vector<SearchRoute::Node> locs;
    std::vector<SearchRoute> routes;
    setupRoutes(locs, routes, solRoutes, data);

    for (size_t client : unplanned)
    {
        SearchRoute::Node *U = &locs[client];
        ProblemData::Client const &clientData = data.location(client);
        assert(!U->route());

        // Determine route with centroid nearest to this client.
        auto const cmp = [&](auto const &a, auto const &b)
        {
            if (a.empty() && !b.empty())
                return false;

            if (b.empty() && !a.empty())
                return true;

            auto const x = clientData.x;
            auto const y = clientData.y;

            auto const [aX, aY] = a.centroid();
            auto const [bX, bY] = b.centroid();

            auto const distA = std::hypot((x - aX).get(), (y - aY).get());
            auto const distB = std::hypot((x - bX).get(), (y - bY).get());
            return distA < distB;
        };

        auto &route = *std::min_element(routes.begin(), routes.end(), cmp);

        // Find best insertion point in selected route, either after a client,
        // or, initially, after the depot.
        Cost bestCost = insertCost(U, route[0], data, costEvaluator);
        auto offset = 1;

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
