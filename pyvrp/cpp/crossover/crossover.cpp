#include "crossover.h"
#include "Measure.h"

#include <cmath>
#include <limits>

using Client = int;
using Route = std::vector<Client>;
using Routes = std::vector<Route>;

namespace
{
// Approximates the cost change of inserting client between prev and next.
Cost deltaCost(Client client,
               Client prev,
               Client next,
               ProblemData const &data,
               CostEvaluator const &costEvaluator)
{
    auto const currDist = data.dist(prev, next);
    auto const propDist = data.dist(prev, client) + data.dist(client, next);

#ifdef VRP_NO_TIME_WINDOWS
    return static_cast<Cost>(propDist - currDist);
#else
    auto const &clientData = data.client(client);
    auto const &prevData = data.client(prev);
    auto const &nextData = data.client(next);

    auto const prevEarly = std::max(data.duration(0, prev), prevData.twEarly);
    auto const prevEarlyFinish = prevEarly + prevData.serviceDuration;
    auto const arriveClient = prevEarlyFinish + data.duration(prev, client);

    auto const clientEarly = std::max(arriveClient, clientData.twEarly);
    auto const clientEarlyFinish = clientEarly + clientData.serviceDuration;
    auto const arriveNext = clientEarlyFinish + data.duration(client, next);

    auto const clientLate = clientData.twLate;
    auto const nextLate = nextData.twLate;
    auto const arrivePrevNext = prevEarlyFinish + data.duration(prev, next);

    auto const currTimeWarp = std::max<Duration>(arrivePrevNext - nextLate, 0);
    auto const propTimeWarp = std::max<Duration>(arriveClient - clientLate, 0)
                              + std::max<Duration>(arriveNext - nextLate, 0);

    return static_cast<Cost>(propDist - currDist)
           + costEvaluator.twPenalty(propTimeWarp)
           - costEvaluator.twPenalty(currTimeWarp);
#endif
}
}  // namespace

void crossover::greedyRepair(Routes &routes,
                             std::vector<Client> const &unplanned,
                             ProblemData const &data,
                             CostEvaluator const &costEvaluator)
{
    // Determine the index just past the last non-empty route.
    auto const pred = [](Route const &route) { return !route.empty(); };
    auto const last = std::find_if(routes.rbegin(), routes.rend(), pred);
    auto const numRoutes = std::distance(last, routes.rend());

    // Determine centroids of each route.
    std::vector<std::pair<double, double>> centroids(numRoutes, {0, 0});
    for (auto rIdx = 0; rIdx != numRoutes; ++rIdx)
        for (Client client : routes[rIdx])
        {
            auto const size = static_cast<double>(routes[rIdx].size());
            auto const x = static_cast<double>(data.client(client).x);
            auto const y = static_cast<double>(data.client(client).y);

            centroids[rIdx].first += x / size;
            centroids[rIdx].second += y / size;
        }

    for (Client client : unplanned)
    {
        auto const x = static_cast<double>(data.client(client).x);
        auto const y = static_cast<double>(data.client(client).y);

        // Determine non-empty route with centroid nearest to this client.
        auto bestDistance = std::numeric_limits<double>::max();
        auto bestRouteIdx = 0;
        for (auto rIdx = 0; rIdx != numRoutes; ++rIdx)
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
        auto &bestRoute = routes[bestRouteIdx];
        Cost bestCost = std::numeric_limits<Cost>::max();
        auto offset = 0;
        for (size_t idx = 0; idx <= bestRoute.size(); ++idx)
        {
            Client prev, next;

            if (idx == 0)  // try after depot
            {
                prev = 0;
                next = bestRoute[0];
            }
            else if (idx == bestRoute.size())  // try before depot
            {
                prev = bestRoute.back();
                next = 0;
            }
            else  // try between [idx - 1] and [idx]
            {
                prev = bestRoute[idx - 1];
                next = bestRoute[idx];
            }

            auto cost = deltaCost(client, prev, next, data, costEvaluator);
            if (cost < bestCost)
            {
                bestCost = cost;
                offset = idx;
            }
        }

        // Update route centroid and insert client into route.
        auto const size = static_cast<double>(bestRoute.size());
        auto const [routeX, routeY] = centroids[bestRouteIdx];
        centroids[bestRouteIdx].first = (routeX * size + x) / (size + 1);
        centroids[bestRouteIdx].second = (routeY * size + y) / (size + 1);
        bestRoute.insert(bestRoute.begin() + offset, client);
    }
}
