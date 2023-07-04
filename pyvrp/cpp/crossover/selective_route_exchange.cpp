#include "crossover.h"

#include <cassert>
#include <cmath>
#include <unordered_set>

using Client = int;
using Clients = std::vector<Client>;
using ClientSet = std::unordered_set<Client>;
using Route = Solution::Route;
using Routes = std::vector<Route>;

namespace
{
// Angle of the given route w.r.t. the centroid of all client locations.
double routeAngle(ProblemData const &data, Route const &route)
{
    // This computes a pseudo-angle that sorts roughly equivalently to the atan2
    // angle, but is much faster to compute. See the following post for details:
    // https://stackoverflow.com/a/16561333/4316405.
    auto const [dataX, dataY] = data.centroid();
    auto const [routeX, routeY] = route.centroid();
    auto const dx = routeX - dataX;
    auto const dy = routeY - dataY;
    return std::copysign(1. - dx / (std::fabs(dx) + std::fabs(dy)), dy);
}

Routes sortByAscAngle(ProblemData const &data, Routes routes)
{
    auto cmp = [&data](Route a, Route b) {
        return routeAngle(data, a) < routeAngle(data, b);
    };

    std::sort(routes.begin(), routes.end(), cmp);
    return routes;
}
}  // namespace

Solution selectiveRouteExchange(
    std::pair<Solution const *, Solution const *> const &parents,
    ProblemData const &data,
    CostEvaluator const &costEvaluator,
    std::pair<size_t, size_t> const startIndices,
    size_t const numMovedRoutes)
{
    // We create two candidate offsprings, both based on parent A:
    // Let A and B denote the set of customers selected from parents A and B
    // Ac and Bc denote the complements: the customers not selected
    // Let v denote union and ^ intersection
    // Parent A: A v Ac
    // Parent B: B v Bc

    // Offspring 1:
    // B and Ac\B, remainder A\B unplanned
    // (note B v (Ac\B) v (A\B) = B v ((Ac v A)\B) = B v Bc = all)
    // Note Ac\B = (A v B)c

    // Offspring 2:
    // A^B and Ac, remainder A\B unplanned
    // (note A^B v Ac v A\B = (A^B v A\B) v Ac = A v Ac = all)

    auto startA = startIndices.first;
    auto startB = startIndices.second;

    size_t nRoutesA = parents.first->numRoutes();
    size_t nRoutesB = parents.second->numRoutes();

    if (startA >= nRoutesA)
        throw std::invalid_argument("Expected startA < nRoutesA.");

    if (startB >= nRoutesB)
        throw std::invalid_argument("Expected startB < nRoutesB.");

    if (numMovedRoutes < 1 || numMovedRoutes > std::min(nRoutesA, nRoutesB))
    {
        auto msg = "Expected numMovedRoutes in [1, min(nRoutesA, nRoutesB)]";
        throw std::invalid_argument(msg);
    }

    // Sort parents' routes by (ascending) polar angle.
    auto const routesA = sortByAscAngle(data, parents.first->getRoutes());
    auto const routesB = sortByAscAngle(data, parents.second->getRoutes());

    ClientSet selectedA;
    ClientSet selectedB;

    // Routes are sorted on polar angle, so selecting adjacent routes in both
    // parents should result in a large overlap when the start indices are
    // close to each other.
    for (size_t r = 0; r < numMovedRoutes; r++)
    {
        auto const &routeA = routesA[(startA + r) % nRoutesA];
        selectedA.insert(routeA.begin(), routeA.end());

        auto const &routeB = routesB[(startB + r) % nRoutesB];
        selectedB.insert(routeB.begin(), routeB.end());
    }

    // For the selection, we want to minimize |A\B| as these need replanning
    while (true)
    {
        // Difference for moving 'left' in parent A
        int differenceALeft = 0;

        for (Client c : routesA[(startA - 1 + nRoutesA) % nRoutesA])
            differenceALeft += !selectedB.contains(c);

        for (Client c : routesA[(startA + numMovedRoutes - 1) % nRoutesA])
            differenceALeft -= !selectedB.contains(c);

        // Difference for moving 'right' in parent A
        int differenceARight = 0;

        for (Client c : routesA[(startA + numMovedRoutes) % nRoutesA])
            differenceARight += !selectedB.contains(c);

        for (Client c : routesA[startA])
            differenceARight -= !selectedB.contains(c);

        // Difference for moving 'left' in parent B
        int differenceBLeft = 0;

        for (Client c : routesB[(startB - 1 + numMovedRoutes) % nRoutesB])
            differenceBLeft += selectedA.contains(c);

        for (Client c : routesB[(startB - 1 + nRoutesB) % nRoutesB])
            differenceBLeft -= selectedA.contains(c);

        // Difference for moving 'right' in parent B
        int differenceBRight = 0;

        for (Client c : routesB[startB])
            differenceBRight += selectedA.contains(c);

        for (Client c : routesB[(startB + numMovedRoutes) % nRoutesB])
            differenceBRight -= selectedA.contains(c);

        int const bestDifference = std::min({differenceALeft,
                                             differenceARight,
                                             differenceBLeft,
                                             differenceBRight});

        if (bestDifference >= 0)  // there are no further improving moves
            break;

        if (bestDifference == differenceALeft)
        {
            for (Client c : routesA[(startA + numMovedRoutes - 1) % nRoutesA])
                selectedA.erase(c);

            startA = (startA - 1 + nRoutesA) % nRoutesA;
            selectedA.insert(routesA[startA].begin(), routesA[startA].end());
        }
        else if (bestDifference == differenceARight)
        {
            for (Client c : routesA[startA])
                selectedA.erase(c);

            startA = (startA + 1) % nRoutesA;

            for (Client c : routesA[(startA + numMovedRoutes - 1) % nRoutesA])
                selectedA.insert(c);
        }
        else if (bestDifference == differenceBLeft)
        {
            for (Client c : routesB[(startB + numMovedRoutes - 1) % nRoutesB])
                selectedB.erase(c);

            startB = (startB - 1 + nRoutesB) % nRoutesB;
            selectedB.insert(routesB[startB].begin(), routesB[startB].end());
        }
        else if (bestDifference == differenceBRight)
        {
            for (Client c : routesB[startB])
                selectedB.erase(c);

            startB = (startB + 1) % nRoutesB;
            for (Client c : routesB[(startB + numMovedRoutes - 1) % nRoutesB])
                selectedB.insert(c);
        }
    }

    // Identify differences between route sets
    ClientSet clientsInSelectedBNotA;
    for (Client c : selectedB)
        if (!selectedA.contains(c))
            clientsInSelectedBNotA.insert(c);

    std::vector<std::vector<Client>> visits1(nRoutesA);
    std::vector<std::vector<Client>> visits2(nRoutesA);

    // Replace selected routes from parent A with routes from parent B
    for (size_t r = 0; r < numMovedRoutes; r++)
    {
        size_t indexA = (startA + r) % nRoutesA;
        size_t indexB = (startB + r) % nRoutesB;

        for (Client c : routesB[indexB])
        {
            visits1[indexA].push_back(c);  // c in B

            if (!clientsInSelectedBNotA.contains(c))
                visits2[indexA].push_back(c);  // c in A^B
        }
    }

    // Move routes from parent A that are kept
    for (size_t r = numMovedRoutes; r < nRoutesA; r++)
    {
        size_t indexA = (startA + r) % nRoutesA;

        for (Client c : routesA[indexA])
        {
            if (!clientsInSelectedBNotA.contains(c))
                visits1[indexA].push_back(c);  // c in Ac\B

            visits2[indexA].push_back(c);  // c in Ac
        }
    }

    // Insert unplanned clients (those that were in the removed routes of A, but
    // not the inserted routes of B).
    Clients unplanned;
    for (Client c : selectedA)
        if (!selectedB.contains(c))
            unplanned.push_back(c);

    crossover::greedyRepair(visits1, unplanned, data, costEvaluator);
    crossover::greedyRepair(visits2, unplanned, data, costEvaluator);

    // Assign correct types to routes (from parents) and filter empty
    std::vector<Solution::Route> routes1;
    routes1.reserve(nRoutesA);
    std::vector<Solution::Route> routes2;
    routes2.reserve(nRoutesA);

    for (size_t r = 0; r < nRoutesA; r++)
    {
        if (!visits1[r].empty())
            routes1.emplace_back(data, visits1[r], routesA[r].vehicleType());
        if (!visits2[r].empty())
            routes2.emplace_back(data, visits2[r], routesA[r].vehicleType());
    }

    Solution sol1{data, routes1};
    Solution sol2{data, routes2};

    auto const cost1 = costEvaluator.penalisedCost(sol1);
    auto const cost2 = costEvaluator.penalisedCost(sol2);
    return cost1 < cost2 ? sol1 : sol2;
}
