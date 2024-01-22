#include "ordered_crossover.h"

#include "DynamicBitset.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace
{
using Client = size_t;

// Depot value, which is never in a route (since it's not a client). We use
// this as filler to account for possibly missing clients.
static constexpr size_t NOT_IN_ROUTE = 0;
}  // namespace

pyvrp::Solution pyvrp::crossover::orderedCrossover(
    std::pair<Solution const *, Solution const *> const &parents,
    ProblemData const &data,
    std::pair<size_t, size_t> const &indices)
{
    assert(data.numVehicles() == 1);
    assert(parents.first->numClients() > 0 && parents.second->numClients() > 0);

    auto const [start, end] = indices;
    auto const numClients = data.numClients();

    DynamicBitset isInserted(data.numLocations());
    std::vector<Client> newRoute(numClients, NOT_IN_ROUTE);

    // Insert the clients from the first route into the new route, from start
    // to end (possibly wrapping around the end of the route).
    size_t insertIdx = start;
    auto const &route1 = parents.first->getRoutes()[0];
    for (; insertIdx % route1.size() != end % route1.size(); ++insertIdx)
    {
        newRoute[insertIdx % numClients] = route1[insertIdx % route1.size()];
        isInserted[route1[insertIdx % route1.size()]] = true;
    }

    // Fill the route with clients from the second parent, in the order of
    // their visits in the second route.
    auto const &route2 = parents.second->getRoutes()[0];
    for (size_t idx = 0; idx != route2.size(); ++idx)
    {
        Client const client = route2[(end + idx) % route2.size()];
        if (!isInserted[client])
        {
            newRoute[insertIdx % numClients] = client;
            insertIdx++;
        }
    }

    // Remove the "not in route" entries from the new route. These were needed
    // because we cannot assume each route has the same clients when optional
    // clients are present.
    std::vector<Client> offspring;
    std::copy_if(newRoute.begin(),
                 newRoute.end(),
                 std::back_inserter(offspring),
                 [](auto client) { return client != NOT_IN_ROUTE; });

    return {data, {offspring}};
}
