#include "greedy_repair.h"
#include "repair.h"

#include "search/primitives.h"

#include <cassert>
#include <limits>

using pyvrp::Solution;
using pyvrp::search::insertCost;
using pyvrp::search::Route;

using Locations = std::vector<Route::Node>;
using Routes = std::vector<Route>;
using SolRoutes = std::vector<Solution::Route>;

std::vector<Solution::Route>
pyvrp::repair::greedyRepair(SolRoutes const &solRoutes,
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
