#include "StringRemoval.h"
#include "primitives.h"

#include <algorithm>
#include <vector>

void pyvrp::search::StringRemoval::operator()(
    PerturbationContext const &context)
{
    if (data_.numClients() == 0)
        return;

    auto const maxPerRoute = 10;

    std::vector<size_t> nodes = {context.orderNodes.front()};
    for (auto const neighbour : context.neighbours[nodes.front()])
        nodes.push_back(neighbour);

    std::vector<size_t> removed;

    size_t numDestroyed = 0;
    for (auto const idx : nodes)
    {
        auto *U = &context.nodes[idx];
        if (!U->route())
            continue;

        auto *route = U->route();
        size_t routeDestroyed = 0;  // Count per route

        while (!U->isDepot() && routeDestroyed < maxPerRoute)
        {
            context.promising[U->client()] = true;
            context.promising[p(U)->client()] = true;
            context.promising[n(U)->client()] = true;
            removed.push_back(U->client());

            auto next = n(U);
            route->remove(U->idx());
            route->update();
            U = next;

            routeDestroyed++;
            if (++numDestroyed == context.numPerturb)
                break;
        }

        if (numDestroyed == context.numPerturb)
            break;
    }
}

pyvrp::search::StringRemoval::StringRemoval(ProblemData const &data)
    : data_(data)
{
}
