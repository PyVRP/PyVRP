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
               [[maybe_unused]] CostEvaluator const &costEvaluator)
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

    // Determine the earliest time we can depart from prev.
    auto const prevStart = std::max(data.duration(0, prev), prevData.twEarly);
    auto const prevFinish = prevStart + prevData.serviceDuration;

    // Time warp when we go directly from prev to next (current situation).
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


void printRouteAsBinary(Route const &route, ProblemData const &data, size_t routeIndex, Client client, Cost cost) {
    std::cout << "                                                  CROSSOVER PRINT Enter" << std::endl;
    std::cout << "                                                            CROSSOVER PRINT ";
    for (Client c : route) {
        bool isDelivery = (data.client(c).demandWeight || data.client(c).demandVolume);
        bool isSalvage = (data.client(c).demandSalvage == 1);
        if (client == c)
            std::cout << "-";
        if (isDelivery && isSalvage) {
            std::cout << "2";
        } else if (isSalvage && !isDelivery) {
            std::cout << "1";
        } else if (isDelivery && !isSalvage) {
            std::cout << "0";
        } else
            std::cout << "X";
        if (client == c)
            std::cout << "-";
    }
    std::cout << std::endl;

    std::cout << "                                                            CROSSOVER PRINT Best Cost[" << cost << "], Route[" << routeIndex << "], Client[" << client << "] " 
              << data.client(client).demandWeight << " " << data.client(client).demandVolume 
              << " " << data.client(client).demandSalvage << std::endl;
    std::cout << "                                                            CROSSOVER PRINT ";
    for (Client c : route) {
        bool isDelivery = (data.client(c).demandWeight || data.client(c).demandVolume);
        bool isSalvage = (data.client(c).demandSalvage == 1);
        if (isDelivery && isSalvage) {
            std::cout << c << ":2 ";
        } else if (isSalvage && !isDelivery) {
            std::cout << c << ":1 ";
        } else if (isDelivery && !isSalvage) {
            std::cout << c << ":0 ";
        } else
            std::cout << c << ":X ";
    }
    std::cout << std::endl;
    std::cout << "                                                  CROSSOVER PRINT Exit" << std::endl;
}

bool checkSequence(ProblemData const &data, Route const &route)
{
    std::cout << "                              CROSSOVER CHECKSEQUENCE Enter" << std::endl;
    bool foundDelivery = false;
    bool foundBoth = false;
    bool foundSalvage = false;

    for (Client c : route) {
        bool isDelivery = (data.client(c).demandWeight || data.client(c).demandVolume);
        bool isSalvage = (data.client(c).demandSalvage == 1);
        bool isBoth = (isDelivery && isSalvage);

        std::cout << "                                        CROSSOVER CHECKSEQUENCE Client: " << c 
           << " Dem Salv: " << data.client(c).demandSalvage 
           << " Dem Weig: " << data.client(c).demandWeight
           << " Dem Volu: " << data.client(c).demandVolume
           << " Salv is: " << isSalvage 
           << " Salv fo: " << foundSalvage << " Both is: " 
           << isBoth << " Both fo: " << foundBoth << " Del is: " 
          << isDelivery << " Del fo: " << isDelivery << std::endl;

        if (isBoth && (foundBoth || foundSalvage))
        { 
            std::cout << "                                        CROSSOVER CHECKSEQUENCE Failed (isBoth && (foundBoth || foundSalvage))" << std::endl;
            std::cout << "                                        CROSSOVER CHECKSEQUENCE Exit checkSequence" << std::endl;
            return false;
        }
        if (isDelivery && (foundBoth || foundSalvage))
        {
            std::cout << "                                        CROSSOVER CHECKSEQUENCE Failed (isDelivery && (foundBoth || foundSalvage))" << std::endl;
            std::cout << "                                        CROSSOVER CHECKSEQUENCE Exit checkSequence" << std::endl;
            return false;
        }
        if (isSalvage && foundBoth)
        {
            std::cout << "                                        CROSSOVER CHECKSEQUENCE Failed (isSalvage && foundBoth)" << std::endl;
            std::cout << "                                        CROSSOVER CHECKSEQUENCE Exit checkSequence" << std::endl;
            return false;
        }

        if (isSalvage)
        {   
            if (!foundSalvage) 
                foundSalvage = true;
        }
        
        if (isDelivery)
        {   
            if (!foundDelivery) 
                foundDelivery = true;
        }
        
        if(isBoth)
        {   
            if (!foundBoth) 
                foundBoth = true;
        }

        continue;
    }
    std::cout << "                              CROSSOVER CHECKSEQUENCE Exit" << std::endl;
    return true;
}

void crossover::reorderRoutes(std::vector<std::vector<Client>> &routes, ProblemData const &data)
{
    std::cout << "                    CROSSOVER REORDER Enter" << std::endl;
    std::vector<std::vector<Client>> newRoutes;

    int routeCnt = 0;

    for (auto &route : routes)
    {
        std::cout << "                              CROSSOVER REORDER Route: " << routeCnt << std::endl;
        std::vector<Client> deliveryRoute, bothRoute, salvageRoute;

        for (const auto &client : route)
        {
            std::cout << "                                        CROSSOVER REORDER  Client: " << client << std::endl;
            bool isDelivery = (data.client(client).demandWeight || data.client(client).demandVolume);
            bool isSalvage = (data.client(client).demandSalvage == 1);
            bool isBoth = (isDelivery && isSalvage);

            if (isDelivery && !isBoth)
            {
                deliveryRoute.push_back(client);
            }
            else if (isBoth)
            {
                bothRoute.push_back(client);
            }
            else
            {
                salvageRoute.push_back(client);
            }
        }

        auto deliveryIter = deliveryRoute.begin();
        auto bothIter = bothRoute.begin();
        auto salvageIter = salvageRoute.begin();

        while (deliveryIter != deliveryRoute.end() || bothIter != bothRoute.end() || salvageIter != salvageRoute.end())
        {
            std::vector<Client> newRoute;

            // Add Ds
            while (deliveryIter != deliveryRoute.end())
            {
                newRoute.push_back(*deliveryIter);
                ++deliveryIter;
            }

            // Add single B if exists
            if (bothIter != bothRoute.end())
            {
                newRoute.push_back(*bothIter);
                ++bothIter;
            }

            newRoutes.push_back(newRoute);

            // If no more Bs exist, add remaining Ss to new route
            if (bothIter == bothRoute.end())
            {
                newRoute.clear();

                while (salvageIter != salvageRoute.end())
                {
                    newRoute.push_back(*salvageIter);
                    ++salvageIter;
                }

                if (!newRoute.empty())
                {
                    newRoutes.push_back(newRoute);
                }
            }
        }
        routeCnt++;
    }
    std::cout << "                              CROSSOVER REORDERROUTES Exit" << std::endl;
    routes = newRoutes;
}


//void crossover::reorderRoutes(std::vector<std::vector<Client>> &routes, ProblemData const &data)
//{
//
//    std::cout << "                              CROSSOVER REORDERROUTES Enter" << std::endl;
//
//    int routeCnt = 0;
//
//    for (auto &route : routes)
//    {
//        std::cout << "                                        CROSSOVER REORDERROUTES Route: " << routeCnt << std::endl;
//        size_t s = 0;
//        Cost t = 0;
//        Client c = 0;
//        printRouteAsBinary(route, data, s, c, t);
//
//        std::vector<int> deliveryRoute, salvageRoute;
//
//        for (const auto &client : route)
//        {
//            std::cout << "                                                  CROSSOVER REORDERROUTES" << " Client: " << client << std::endl;
//            printRouteAsBinary(route, data, s, client, t);
//            bool isDelivery = (data.client(client).demandWeight || data.client(client).demandVolume);
//            bool isSalvage = (data.client(client).demandSalvage == 1);
//            bool isBoth = (isDelivery && isSalvage);
//
//            if (isDelivery && !isBoth)
//            {
//                deliveryRoute.push_back(client);
//            }
//            else 
//            {
//                salvageRoute.push_back(client);
//            }
//        }
//
//        route.clear();
//        route.insert(route.end(), deliveryRoute.begin(), deliveryRoute.end());
//        route.insert(route.end(), salvageRoute.begin(), salvageRoute.end());
//        printRouteAsBinary(route, data, s, c, t);
//        routeCnt++;
//    }
//    std::cout << "                              CROSSOVER REORDERROUTES Exit" << std::endl;
//}

//void crossover::reorderRoutes(std::vector<std::vector<Client>> &routes, ProblemData const &data)
//{
//    std::cout << "                              CROSSOVER REORDERROUTES Enter" << std::endl;
//    int routeCnt = 0;
//
//    std::vector<std::vector<Client>> newRoutes;
//
//    for (auto &route : routes)
//    {
//        std::cout << "                                        CROSSOVER REORDERROUTES Route: " << routeCnt << std::endl;
//        size_t s = 0;
//        Cost t = 0;
//        Client c = 0;
//        printRouteAsBinary(route, data, s, c, t);
//
//        std::vector<int> deliveryRoute, bothRoute, salvageRoute;
//
//        for (const auto &client : route)
//        {
//            std::cout << "                                                  CROSSOVER REORDERROUTES" << " Client: " << client << std::endl;
//            printRouteAsBinary(route, data, s, client, t);
//            bool isDelivery = (data.client(client).demandWeight || data.client(client).demandVolume);
//            bool isSalvage = (data.client(client).demandSalvage == 1);
//            bool isBoth = (isDelivery && isSalvage);
//
//            if (isDelivery && !isBoth)
//            {
//                deliveryRoute.push_back(client);
//            }
//            else if (isBoth)
//            {
//                bothRoute.push_back(client);
//            }
//            else
//            {
//                salvageRoute.push_back(client);
//            }
//        }
//
//        if (!deliveryRoute.empty())
//        {
//            newRoutes.push_back(deliveryRoute);
//        }
//        if (!bothRoute.empty())
//        {
//            if (!newRoutes.back().empty())
//            {
//                newRoutes.back().insert(newRoutes.back().end(), bothRoute.begin(), bothRoute.end());
//            }
//            else
//            {
//                newRoutes.push_back(bothRoute);
//            }
//        }
//        if (!salvageRoute.empty())
//        {
//            if (!newRoutes.back().empty())
//            {
//                newRoutes.back().insert(newRoutes.back().end(), salvageRoute.begin(), salvageRoute.end());
//            }
//            else
//            {
//                newRoutes.push_back(salvageRoute);
//            }
//        }
//        routeCnt++;
//    }
//    routes = newRoutes;
//    std::cout << "                              CROSSOVER REORDERROUTES Exit" << std::endl;
//}

void crossover::greedyRepair(Routes &routes,
                             std::vector<Client> const &unplanned,
                             ProblemData const &data,
                             CostEvaluator const &costEvaluator)
{
    std::cout << "                    CROSSOVER GREEDY Enter" << std::endl;
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

    for (Client client : unplanned)
    {
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
        auto &bestRoute = routes[bestRouteIdx];
        Cost bestCost = std::numeric_limits<Cost>::max();
        auto offset = 0;
        for (size_t idx = 0; idx <= bestRoute.size(); ++idx)
        {
            Client prev, next;

            if (idx != 0)
            {
                prev = bestRoute[idx - 1];
            }

            if (idx != bestRoute.size())
            {
                next = bestRoute[idx];
            }

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

            Route potentialRoute = bestRoute;
            potentialRoute.insert(potentialRoute.begin() + idx, client);
            bool isValid = checkSequence(data, potentialRoute);

            if (isValid)
            {
                if (cost < bestCost)
                {
                    std::cout << "                              CROSSOVER GREEDY No violation, New best cost found at idx " << idx << ", Cost: " << cost << std::endl;
                    printRouteAsBinary(potentialRoute, data, bestRouteIdx, client, cost);
                    bestCost = cost;
                    offset = idx;
                }
                else
                {
                    std::cout << "                              CROSSOVER GREEDY No violation, But higher cost at idx " << idx << ", Cost: " << cost << std::endl;
                    printRouteAsBinary(potentialRoute, data, bestRouteIdx, client, bestCost);
                }
            }
            else
            {
                std::cout << "                              CROSSOVER GREEDY Violation at idx " << idx << ", Cost: " << cost << std::endl;
                printRouteAsBinary(potentialRoute, data, bestRouteIdx, client, bestCost);
                bestCost = std::numeric_limits<Cost>::max(); // Reset the bestCost if sequence is invalid
                // offset = -1; // Set offset to -1 to indicate no valid insertion point found
            }
        }

        // Update route centroid and insert client into route.
        auto const size = static_cast<double>(bestRoute.size());
        auto const [routeX, routeY] = centroids[bestRouteIdx];
        centroids[bestRouteIdx].first = (routeX * size + x) / (size + 1);
        centroids[bestRouteIdx].second = (routeY * size + y) / (size + 1);
        bestRoute.insert(bestRoute.begin() + offset, client);

        std::cout << "                              CROSSOVER GREEDY Final best route: " << std::endl;
        printRouteAsBinary(bestRoute, data, bestRouteIdx, client, bestCost);
    }
    std::cout << "                    CROSSOVER GREEDY Exit" << std::endl;
}
