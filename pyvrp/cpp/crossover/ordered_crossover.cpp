#include "ordered_crossover.h"

#include "DynamicBitset.h"

#include <cassert>
#include <vector>

using Client = size_t;

pyvrp::Solution pyvrp::crossover::orderedCrossover(
    std::pair<Solution const *, Solution const *> const &parents,
    ProblemData const &data,
    std::pair<size_t, size_t> const indices)
{
    assert(data.numVehicles() == 1);
    auto const numClients = data.numClients();

    auto const [start, end] = indices;
    auto const &route1 = parents.first->getRoutes()[0];
    auto const &route2 = parents.second->getRoutes()[0];

    DynamicBitset isInserted(data.numLocations());  // tracks insertion
    std::vector<Client> newRoute(numClients);

    // Insert the clients from the first route into the new route, from start
    // to end (possibly wrapping around the end of the route).
    size_t newIdx = start;
    for (; newIdx % numClients != end % numClients; ++newIdx)
    {
        newRoute[newIdx % numClients] = route1[newIdx % numClients];
        isInserted[route1[newIdx % numClients]] = true;
    }

    // Fill the remaining clients in the order given by the second parent.
    for (size_t idx = 0; idx != numClients; ++idx)
    {
        Client const client = route2[(end + idx) % numClients];
        if (!isInserted[client])
        {
            newRoute[newIdx % numClients] = client;
            newIdx++;
        }
    }

    return {data, {newRoute}};
}
