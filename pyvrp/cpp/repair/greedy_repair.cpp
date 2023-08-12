#include "greedy_repair.h"

#include "Exchange.h"

#include <cassert>

using pyvrp::CostEvaluator;
using pyvrp::DynamicBitset;
using pyvrp::ProblemData;
using pyvrp::Solution;
using pyvrp::search::Exchange;
using pyvrp::search::p;
using pyvrp::search::Route;

using Clients = std::vector<Route::Node>;
using Routes = std::vector<Route>;

void setupRoutes(Solution const &sol,
                 ProblemData const &data,
                 Clients &clients,
                 Routes &routes)
{
    assert(clients.empty() && routes.empty());

    for (size_t client = 0; client <= data.numClients(); ++client)
        clients.emplace_back(client);

    size_t idx = 0;
    for (auto const &solRoute : sol.getRoutes())
    {
        auto &route = routes.emplace_back(data, idx++, solRoute.vehicleType());
        for (auto const client : solRoute.visits())
            route.push_back(&clients[client]);
    }

    for (auto &route : routes)
        route.update();
}

Solution exportRoutes(ProblemData const &data, Routes const &routes)
{
    std::vector<Solution::Route> solRoutes;

    for (auto const &route : routes)
    {
        std::vector<size_t> visits;
        for (auto *node : route)
            visits.push_back(node->client());

        solRoutes.emplace_back(data, visits, route.vehicleType());
    }

    return {data, solRoutes};
}

Solution pyvrp::repair::greedyRepair(Solution const &solution,
                                     DynamicBitset const &unplanned,
                                     ProblemData const &data,
                                     CostEvaluator const &costEvaluator)
{
    Clients clients;
    Routes routes;
    setupRoutes(solution, data, clients, routes);

    Exchange<1, 0> move(data);

    for (size_t client = 1; client <= data.numClients(); ++client)
    {
        if (!unplanned[client])
            continue;

        Route::Node *U = &clients[client];
        assert(!U->route());

        Route::Node *UAfter = nullptr;
        pyvrp::Cost deltaCost = std::numeric_limits<pyvrp::Cost>::max();

        for (size_t other = 1; other <= data.numClients(); ++other)
        {
            if (unplanned[other] || other == client)
                continue;

            auto *V = &clients[other];
            assert(V->route());

            auto const cost = move.evaluate(U, V, costEvaluator);
            if (cost < deltaCost)
            {
                deltaCost = cost;
                UAfter = V;
            }

            if (p(V)->isDepot())
            {
                auto const cost = move.evaluate(U, p(V), costEvaluator);
                if (cost < deltaCost)
                {
                    deltaCost = cost;
                    UAfter = V;
                }
            }
        }

        move.apply(U, UAfter);
        UAfter->route()->update();
    }

    return exportRoutes(data, routes);
}
