#include "greedy_repair.h"

#include "TimeWindowSegment.h"
#include "search/Route.h"

#include <cassert>

using pyvrp::CostEvaluator;
using pyvrp::ProblemData;
using pyvrp::Solution;
using pyvrp::search::n;
using pyvrp::search::p;
using pyvrp::search::Route;

using Clients = std::vector<Route::Node>;
using Routes = std::vector<Route>;
using SolRoutes = std::vector<Solution::Route>;
using TWS = pyvrp::TimeWindowSegment;

namespace
{
// Populate the given clients and routes vectors with routes from the solution.
void setupRoutes(Clients &clients,
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

// Evaluates the move of inserting U after V.
pyvrp::Cost evaluate(Route::Node *U,
                     Route::Node *V,
                     ProblemData const &data,
                     CostEvaluator const &costEvaluator)
{
    assert(!U->route() && V->route());
    auto *route = V->route();

    auto const deltaDist = data.dist(V->client(), U->client())
                           + data.dist(U->client(), n(V)->client())
                           - data.dist(V->client(), n(V)->client());

    auto const &uClient = data.client(U->client());
    auto deltaCost = static_cast<pyvrp::Cost>(deltaDist) - uClient.prize;

    deltaCost += costEvaluator.loadPenalty(route->load() + uClient.demand,
                                           route->capacity());
    deltaCost -= costEvaluator.loadPenalty(route->load(), route->capacity());

    // If this is true, adding U cannot decrease time warp in V's route enough
    // to offset the deltaCost.
    if (deltaCost >= costEvaluator.twPenalty(route->timeWarp()))
        return deltaCost;

    auto const vTWS = TWS::merge(data.durationMatrix(),
                                 route->twsBefore(V->idx()),
                                 TWS(U->client(), uClient),
                                 route->twsAfter(V->idx() + 1));

    deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(route->timeWarp());

    return deltaCost;
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

    Clients clients;
    Routes routes;
    setupRoutes(clients, routes, solRoutes, data);

    for (auto const client : unplanned)
    {
        Route::Node *U = &clients[client];
        assert(!U->route());

        Route::Node *UAfter = nullptr;
        pyvrp::Cost deltaCost = std::numeric_limits<pyvrp::Cost>::max();

        for (auto &route : routes)
        {
            auto const cost = evaluate(U, route[0], data, costEvaluator);
            if (cost < deltaCost)  // evaluate after depot
            {
                deltaCost = cost;
                UAfter = route[0];
            }

            for (auto *V : route)  // evaluate after V
            {
                auto const cost = evaluate(U, V, data, costEvaluator);
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
