#include "ordered_crossover.h"

#include "DynamicBitset.h"

#include <algorithm>
#include <cassert>
#include <vector>

using Client = size_t;

pyvrp::Solution pyvrp::crossover::orderedCrossover(
    std::pair<Solution const *, Solution const *> const &parents,
    ProblemData const &data,
    std::pair<size_t, size_t> const indices)
{
    assert(data.numVehicles() == 1);

    auto const [start, end] = indices;
    auto const &route1 = parents.first->getRoutes()[0];
    auto const &route2 = parents.second->getRoutes()[0];

    DynamicBitset isInserted(data.numLocations());       // tracks insertion
    std::vector<Client> newRoute(data.numClients(), 0);  // 0 is a filler value

    // Insert the clients from the first route into the new route, from start
    // to end (possibly wrapping around the end of the route).
    size_t newIdx = start;
    for (; newIdx % route1.size() != end % route1.size(); ++newIdx)
    {
        newRoute[newIdx % data.numClients()] = route1[newIdx % route1.size()];
        isInserted[route1[newIdx % route1.size()]] = true;
    }

    // Fill the route with clients from the second parent, in the order of
    // their visits in the second route.
    for (size_t idx = 0; idx != route2.size(); ++idx)
    {
        Client const client = route2[(end + idx) % route2.size()];
        if (!isInserted[client])
        {
            newRoute[newIdx % data.numClients()] = client;
            newIdx++;
        }
    }

    // Remove the filler from the new route. The filler was needed because we
    // cannot assume each route has the same clients when optional clients are
    // present. We do know that depots are never in the route's client list,
    // and thus that 0 is a safe filler value.
    std::vector<Client> cleanRoute;
    std::copy_if(newRoute.begin(),
                 newRoute.end(),
                 std::back_inserter(cleanRoute),
                 [](auto client) { return client > 0; });

    return {data, {cleanRoute}};
}
