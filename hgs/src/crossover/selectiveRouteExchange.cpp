#include "crossover.h"

#include <unordered_set>

using Client = int;
using Clients = std::vector<Client>;
using ClientSet = std::unordered_set<Client>;
using Route = std::vector<Client>;
using Routes = std::vector<Route>;

Individual selectiveRouteExchange(
    std::pair<Individual const *, Individual const *> const &parents,
    Params const &params,
    XorShift128 &rng)
{
    size_t nRoutesA = parents.first->numRoutes();
    size_t nRoutesB = parents.second->numRoutes();

    // Picking the start index of routes to replace of parent A. We like to
    // replace routes with a large overlap of tasks, so we choose adjacent
    // routes (they are sorted on polar angle by the local search/educate step).
    size_t startA = rng.randint(nRoutesA);
    size_t minRoutes = std::min(nRoutesA, nRoutesB);
    size_t nMovedRoutes = rng.randint(minRoutes) + 1;  // at least one
    size_t startB = startA < nRoutesB ? startA : 0;

    auto const &routesA = parents.first->getRoutes();
    auto const &routesB = parents.second->getRoutes();

    ClientSet selectedA;
    ClientSet selectedB;

    for (size_t r = 0; r < nMovedRoutes; r++)
    {
        selectedA.insert(routesA[(startA + r) % nRoutesA].begin(),
                         routesA[(startA + r) % nRoutesA].end());

        selectedB.insert(routesB[(startB + r) % nRoutesB].begin(),
                         routesB[(startB + r) % nRoutesB].end());
    }

    while (true)
    {
        // Difference for moving 'left' in parent A
        int differenceALeft = 0;

        for (Client c : routesA[(startA - 1 + nRoutesA) % nRoutesA])
            differenceALeft += !selectedB.contains(c);

        for (Client c : routesA[(startA + nMovedRoutes - 1) % nRoutesA])
            differenceALeft -= !selectedB.contains(c);

        // Difference for moving 'right' in parent A
        int differenceARight = 0;

        for (Client c : routesA[(startA + nMovedRoutes) % nRoutesA])
            differenceARight += !selectedB.contains(c);

        for (Client c : routesA[startA])
            differenceARight -= !selectedB.contains(c);

        // Difference for moving 'left' in parent B
        int differenceBLeft = 0;

        for (Client c : routesB[(startB - 1 + nMovedRoutes) % nRoutesB])
            differenceBLeft += selectedA.contains(c);

        for (Client c : routesB[(startB - 1 + nRoutesB) % nRoutesB])
            differenceBLeft -= selectedA.contains(c);

        // Difference for moving 'right' in parent B
        int differenceBRight = 0;

        for (Client c : routesB[startB])
            differenceBRight += selectedA.contains(c);

        for (Client c : routesB[(startB + nMovedRoutes) % nRoutesB])
            differenceBRight -= selectedA.contains(c);

        int const bestDifference = std::min({differenceALeft,
                                             differenceARight,
                                             differenceBLeft,
                                             differenceBRight});

        if (bestDifference >= 0)  // there are no further improving moves
            break;

        if (bestDifference == differenceALeft)
        {
            for (Client c : routesA[(startA + nMovedRoutes - 1) % nRoutesA])
                selectedA.erase(c);

            startA = (startA - 1 + nRoutesA) % nRoutesA;
            selectedA.insert(routesA[startA].begin(), routesA[startA].end());
        }
        else if (bestDifference == differenceARight)
        {
            for (Client c : routesA[startA])
                selectedA.erase(c);

            startA = (startA + 1) % nRoutesA;

            for (Client c : routesA[(startA + nMovedRoutes - 1) % nRoutesA])
                selectedA.insert(c);
        }
        else if (bestDifference == differenceBLeft)
        {
            for (Client c : routesB[(startB + nMovedRoutes - 1) % nRoutesB])
                selectedB.erase(c);

            startB = (startB - 1 + nRoutesB) % nRoutesB;
            selectedB.insert(routesB[startB].begin(), routesB[startB].end());
        }
        else if (bestDifference == differenceBRight)
        {
            for (Client c : routesB[startB])
                selectedB.erase(c);

            startB = (startB + 1) % nRoutesB;
            for (Client c : routesB[(startB + nMovedRoutes - 1) % nRoutesB])
                selectedB.insert(c);
        }
    }

    // Identify differences between route sets
    ClientSet clientsInSelectedBNotA;
    for (Client c : selectedB)
        if (!selectedA.contains(c))
            clientsInSelectedBNotA.insert(c);

    Routes routes1(params.nbVehicles);
    Routes routes2(params.nbVehicles);

    // Replace selected routes from parent A with routes from parent B
    for (size_t r = 0; r < nMovedRoutes; r++)
    {
        size_t indexA = (startA + r) % nRoutesA;
        size_t indexB = (startB + r) % nRoutesB;

        for (Client c : routesB[indexB])
        {
            routes1[indexA].push_back(c);

            if (!clientsInSelectedBNotA.contains(c))
                routes2[indexA].push_back(c);
        }
    }

    // Move routes from parent A that are kept
    for (size_t r = nMovedRoutes; r < nRoutesA; r++)
    {
        size_t indexA = (startA + r) % nRoutesA;

        for (Client c : routesA[indexA])
        {
            if (!clientsInSelectedBNotA.contains(c))
                routes1[indexA].push_back(c);

            routes2[indexA].push_back(c);
        }
    }

    // Insert unplanned clients (those that were in the removed routes of A, but
    // not the inserted routes of B).
    Clients unplanned;
    for (Client c : selectedA)
        if (!selectedB.contains(c))
            unplanned.push_back(c);

    crossover::greedyRepair(routes1, unplanned, params);
    crossover::greedyRepair(routes2, unplanned, params);

    Individual indiv1{&params, routes1};
    Individual indiv2{&params, routes2};

    return std::min(indiv1, indiv2);
}
