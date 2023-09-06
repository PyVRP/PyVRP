#include "greedy_repair.h"

#include "TimeWindowSegment.h"
#include "search/Route.h"
#include "search/primitives.h"

#include <cassert>

using pyvrp::CostEvaluator;
using pyvrp::ProblemData;
using pyvrp::Solution;
using pyvrp::search::insertCost;
using pyvrp::search::Route;

using Locations = std::vector<Route::Node>;
using Routes = std::vector<Route>;
using SolRoutes = std::vector<Solution::Route>;

namespace
{
// Populate the given locs and routes vectors with routes from the solution.
void setupRoutes(Locations &locs,
                 Routes &routes,
                 SolRoutes const &solRoutes,
                 ProblemData const &data)
{
    assert(locs.empty() && routes.empty());

    // Doing this avoids re-allocations, which would break the pointer structure
    // that Route and Route::Node use.
    locs.reserve(data.numLocations());
    routes.reserve(solRoutes.size());

    for (size_t loc = 0; loc != data.numLocations(); ++loc)
        locs.emplace_back(loc);

    size_t idx = 0;
    for (auto const &solRoute : solRoutes)
    {
        auto &route = routes.emplace_back(data, idx++, solRoute.vehicleType());
        for (auto const client : solRoute)
            route.push_back(&locs[client]);

        route.update();
    }
}

// Turns the given routes into a solution.
Solution exportRoutes(ProblemData const &data, Routes const &routes)
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
}  // namespace

Solution pyvrp::repair::greedyRepair(Solution const &solution,
                                     std::vector<size_t> const &unplanned,
                                     ProblemData const &data,
                                     CostEvaluator const &costEvaluator)
{
    return greedyRepair(solution.getRoutes(), unplanned, data, costEvaluator);
}

Solution pyvrp::repair::greedyRepair(SolRoutes const &solRoutes,
                                     std::vector<size_t> const &unplanned,
                                     ProblemData const &data,
                                     CostEvaluator const &costEvaluator)
{
    if (solRoutes.empty() && !unplanned.empty())
        throw std::invalid_argument("Need routes to repair!");

    Locations locs;
    Routes routes;
    setupRoutes(locs, routes, solRoutes, data);

    for (auto const client : unplanned)
    {
        Route::Node *U = &locs[client];
        assert(!U->route());

        Route::Node *UAfter = nullptr;
        pyvrp::Cost deltaCost = std::numeric_limits<pyvrp::Cost>::max();

        for (auto &route : routes)
        {
            auto const cost = insertCost(U, route[0], data, costEvaluator);
            if (cost < deltaCost)  // evaluate after depot
            {
                deltaCost = cost;
                UAfter = route[0];
            }

            for (auto *V : route)  // evaluate after V
            {
                auto const cost = insertCost(U, V, data, costEvaluator);
                if (cost < deltaCost)
                {
                    deltaCost = cost;
                    UAfter = V;
                }
            }
        }

        assert(UAfter && UAfter->route());
        UAfter->route()->insert(UAfter->idx() + 1, U);
        UAfter->route()->update();
    }

    return exportRoutes(data, routes);
}
