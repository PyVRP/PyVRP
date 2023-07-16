#include "crossover.h"
#include "Measure.h"

#include <cmath>
#include <limits>

using pyvrp::Cost;
using pyvrp::Duration;

using Client = int;
using Route = std::vector<Client>;
using Routes = std::vector<Route>;

namespace
{
// Approximates the cost change of inserting client between prev and next.
Cost deltaCost(Client client,
               Client prev,
               Client next,
               pyvrp::ProblemData const &data,
               [[maybe_unused]] pyvrp::CostEvaluator const &costEvaluator)
{
    auto const currDist = data.dist(prev, next);
    auto const propDist = data.dist(prev, client) + data.dist(client, next);

#ifdef PYVRP_NO_TIME_WINDOWS
    return static_cast<Cost>(propDist - currDist);
#else
    auto const &clientData = data.client(client);
    auto const &prevData = data.client(prev);

    auto const clientService = clientData.serviceDuration;
    auto const clientLate = clientData.twLate;
    auto const nextLate = data.client(next).twLate;

    // Determine the earliest time we can depart from prev, and compute the
    // time warp when we go directly from prev to next (current situation).
    auto const prevFinish = prevData.twEarly + prevData.serviceDuration;
    auto const prevNextArrive = prevFinish + data.duration(prev, next);
    auto const currTimeWarp = std::max<Duration>(prevNextArrive - nextLate, 0);

    // Determine arrival at client. This incurs some timewarp if the arrival is
    // after client.twLate. We finish at start time + service. We subtract any
    // time warp from the departure time at client.
    auto const clientArrive = prevFinish + data.duration(prev, client);
    auto const clientStart = std::max(clientArrive, clientData.twEarly);
    auto const clientTimeWarp = std::max<Duration>(clientStart - clientLate, 0);
    auto const clientFinish = clientStart - clientTimeWarp + clientService;

    // We then continue to next, where we might arrive too late and also incur
    // further time warp.
    auto const nextArrive = clientFinish + data.duration(client, next);
    auto const nextTimeWarp = std::max<Duration>(nextArrive - nextLate, 0);

    return static_cast<Cost>(propDist - currDist)
           + costEvaluator.twPenalty(clientTimeWarp + nextTimeWarp)  // proposed
           - costEvaluator.twPenalty(currTimeWarp);                  // current
#endif
}
}  // namespace

void pyvrp::crossover::greedyRepair(Routes &routes,
                                    pyvrp::DynamicBitset const &unplanned,
                                    pyvrp::ProblemData const &data,
                                    pyvrp::CostEvaluator const &costEvaluator)
{
    auto const numRoutes = routes.size();

    // Determine centroids of each route.
    std::vector<std::pair<double, double>> centroids(numRoutes, {0, 0});
    for (size_t rIdx = 0; rIdx != numRoutes; ++rIdx)
        for (Client client : routes[rIdx])
        {
            auto const size = static_cast<double>(routes[rIdx].size());
            auto const x = static_cast<double>(data.client(client).x);
            auto const y = static_cast<double>(data.client(client).y);

            centroids[rIdx].first += x / size;
            centroids[rIdx].second += y / size;
        }

    for (size_t client = 1; client != data.numClients() + 1; ++client)
    {
        if (!unplanned[client])
            continue;

        auto const x = static_cast<double>(data.client(client).x);
        auto const y = static_cast<double>(data.client(client).y);

        // Determine non-empty route with centroid nearest to this client.
        auto bestDistance = std::numeric_limits<double>::max();
        auto bestRouteIdx = 0;
        for (size_t rIdx = 0; rIdx != numRoutes; ++rIdx)
        {
            if (routes[rIdx].empty())
                continue;

            auto const distance = std::hypot(x - centroids[rIdx].first,
                                             y - centroids[rIdx].second);

            if (distance < bestDistance)
            {
                bestRouteIdx = rIdx;
                bestDistance = distance;
            }
        }

        // Find best insertion point in selected route.
        Cost bestCost = std::numeric_limits<Cost>::max();
        auto &route = routes[bestRouteIdx];
        auto offset = 0;
        for (size_t idx = 0; idx <= route.size() && !route.empty(); ++idx)
        {
            Client prev, next;

            if (idx == 0)  // try after depot
            {
                prev = 0;  // TODO hardcoded depot
                next = route[0];
            }
            else if (idx == route.size())  // try before depot
            {
                prev = route.back();
                next = 0;  // TODO hardcoded depot
            }
            else  // try between [idx - 1] and [idx]
            {
                prev = route[idx - 1];
                next = route[idx];
            }

            auto cost = deltaCost(client, prev, next, data, costEvaluator);
            if (cost < bestCost)
            {
                bestCost = cost;
                offset = idx;
            }
        }

        // Update route centroid and insert client into route.
        auto const size = static_cast<double>(route.size());
        auto const [routeX, routeY] = centroids[bestRouteIdx];
        centroids[bestRouteIdx].first = (routeX * size + x) / (size + 1);
        centroids[bestRouteIdx].second = (routeY * size + y) / (size + 1);
        route.insert(route.begin() + offset, client);
    }
}
