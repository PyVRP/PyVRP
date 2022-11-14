#include "crossover.h"

using Client = int;
using Route = std::vector<Client>;
using Routes = std::vector<Route>;

namespace
{
struct InsertPos  // best insert position, used to plan unplanned clients
{
    int deltaCost;
    Route *route;
    size_t offset;
};

// Evaluates the cost change of inserting client between prev and next.
int deltaCost(Client client, Client prev, Client next, Params const &params)
{
    int prevClientRelease = std::max(params.clients[prev].releaseTime,
                                     params.clients[client].releaseTime);
    int prevEarliestArrival = std::max(prevClientRelease + params.dist(0, prev),
                                       params.clients[prev].twEarly);
    int prevEarliestFinish = prevEarliestArrival + params.clients[prev].servDur;
    int distPrevClient = params.dist(prev, client);
    int clientLate = params.clients[client].twLate;

    if (prevEarliestFinish + distPrevClient >= clientLate)
        return INT_MAX;

    int clientNextRelease = std::max(params.clients[client].releaseTime,
                                     params.clients[next].releaseTime);
    int clientEarliestArrival
        = std::max(clientNextRelease + params.dist(0, client),
                   params.clients[client].twEarly);
    int clientEarliestFinish
        = clientEarliestArrival + params.clients[client].servDur;
    int distClientNext = params.dist(client, next);
    int nextLate = params.clients[next].twLate;

    if (clientEarliestFinish + distClientNext >= nextLate)
        return INT_MAX;

    return distPrevClient + distClientNext - params.dist(prev, next);
}
}  // namespace

void crossover::greedyRepair(Routes &routes,
                             std::vector<Client> const &unplanned,
                             Params const &params)
{
    size_t numRoutes = 0;  // points just after the last non-empty route
    for (size_t rIdx = 0; rIdx != routes.size(); ++rIdx)
        if (!routes[rIdx].empty())
            numRoutes = rIdx + 1;

    for (Client client : unplanned)
    {
        InsertPos best = {INT_MAX, &routes.front(), 0};

        for (size_t rIdx = 0; rIdx != numRoutes; ++rIdx)
        {
            auto &route = routes[rIdx];

            for (size_t idx = 0; idx <= route.size() && !route.empty(); ++idx)
            {
                Client prev, next;

                if (idx == 0)  // try after depot
                {
                    prev = 0;
                    next = route[0];
                }
                else if (idx == route.size())  // try before depot
                {
                    prev = route.back();
                    next = 0;
                }
                else  // try between [idx - 1] and [idx]
                {
                    prev = route[idx - 1];
                    next = route[idx];
                }

                auto const cost = deltaCost(client, prev, next, params);
                if (cost < best.deltaCost)
                    best = {cost, &route, idx};
            }
        }

        auto const [_, route, offset] = best;
        route->insert(route->begin() + static_cast<long>(offset), client);
    }
}
