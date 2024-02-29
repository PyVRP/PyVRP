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
static constexpr size_t UNUSED = 0;
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

    // New route. This route is initially empty, indicated by all UNUSED
    // values. Any such values that remain after crossover are filtered away.
    std::vector<Client> newRoute(numClients, UNUSED);
    DynamicBitset isInserted(data.numLocations());  // tracks inserted clients

    // Insert the clients from the first route into the new route, from start
    // to end (possibly wrapping around the end of the route).
    size_t insertIdx = start;
    auto const &route1 = parents.first->routes()[0];
    for (; insertIdx % route1.size() != end % route1.size(); ++insertIdx)
    {
        newRoute[insertIdx % numClients] = route1[insertIdx % route1.size()];
        isInserted[route1[insertIdx % route1.size()]] = true;
    }

    // Fill the route with clients from the second parent, in the order of
    // their visits in the second route.
    auto const &route2 = parents.second->routes()[0];
    for (size_t idx = 0; idx != route2.size(); ++idx)
    {
        Client const client = route2[(end + idx) % route2.size()];
        if (!isInserted[client])
        {
            newRoute[insertIdx % numClients] = client;
            insertIdx++;
        }
    }

    // Remove the UNUSED values from the new route. These were needed because
    // we cannot assume both parent solutions have all the same clients (for
    // example, solutions to instances with optional clients typically do not).
    std::vector<Client> offspring;
    std::copy_if(newRoute.begin(),
                 newRoute.end(),
                 std::back_inserter(offspring),
                 [](auto client) { return client != UNUSED; });

    return {data, {offspring}};
}
