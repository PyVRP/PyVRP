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
int deltaCost(Client client, Client prev, Client next, ProblemData const &data)
{
    int prevClientRelease = std::max(data.client(prev).releaseTime,
                                     data.client(client).releaseTime);
    int prevEarliestArrival = std::max(prevClientRelease + data.dist(0, prev),
                                       data.client(prev).twEarly);
    int prevEarliestFinish = prevEarliestArrival + data.client(prev).servDur;
    int distPrevClient = data.dist(prev, client);
    int clientLate = data.client(client).twLate;

    if (prevEarliestFinish + distPrevClient >= clientLate)
        return INT_MAX;

    int clientNextRelease = std::max(data.client(client).releaseTime,
                                     data.client(next).releaseTime);
    int clientEarliestArrival = std::max(
        clientNextRelease + data.dist(0, client), data.client(client).twEarly);
    int clientEarliestFinish
        = clientEarliestArrival + data.client(client).servDur;
    int distClientNext = data.dist(client, next);
    int nextLate = data.client(next).twLate;

    if (clientEarliestFinish + distClientNext >= nextLate)
        return INT_MAX;

    return distPrevClient + distClientNext - data.dist(prev, next);
}
}  // namespace

void crossover::greedyRepair(Routes &routes,
                             std::vector<Client> const &unplanned,
                             ProblemData const &data)
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

                auto const cost = deltaCost(client, prev, next, data);
                if (cost < best.deltaCost)
                    best = {cost, &route, idx};
            }
        }

        auto const [_, route, offset] = best;
        route->insert(route->begin() + static_cast<long>(offset), client);
    }
}
