#include "crossover.h"

using Client = int;
using Route = std::vector<Client>;
using Routes = std::vector<Route>;

namespace
{
struct InsertPos  // best insert position, used to plan unplanned clients
{
    TCost deltaCost;
    Route *route;
    size_t offset;
};

// Evaluates the cost change of inserting client between prev and next.
TCost deltaCost(Client client, Client prev, Client next, ProblemData const &data)
{
    TTime prevClientRelease = std::max(data.client(prev).releaseTime,
                                     data.client(client).releaseTime);
    TTime prevEarliestArrival = std::max(prevClientRelease + data.duration(0, prev),
                                       data.client(prev).twEarly);
    TTime prevEarliestFinish = prevEarliestArrival + data.client(prev).servDur;
    TTime clientLate = data.client(client).twLate;

    if (prevEarliestFinish + data.duration(prev, client) >= clientLate)
        return static_cast<TCost>(INT_MAX);

    TTime clientNextRelease = std::max(data.client(client).releaseTime,
                                     data.client(next).releaseTime);
    TTime clientEarliestArrival = std::max(
        clientNextRelease + data.duration(0, client), data.client(client).twEarly);
    TTime clientEarliestFinish
        = clientEarliestArrival + data.client(client).servDur;
    int nextLate = data.client(next).twLate;

    if (clientEarliestFinish + data.duration(client, next) >= nextLate)
        return INT_MAX;

    return data.dist(prev, client) + data.dist(client, next) - data.dist(prev, next);
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
        InsertPos best = {static_cast<TCost>(INT_MAX), &routes.front(), 0};

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
