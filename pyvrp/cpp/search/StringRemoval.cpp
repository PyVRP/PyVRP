#include "StringRemoval.h"

void pyvrp::search::StringRemoval::operator()(
    PerturbationContext const &context)
{
    if (numPerturb_ == 0 || data_.numClients() == 0)
        return;

    auto const center = context.orderNodes[0];
    auto const &neighbours = context.neighbours[center];
    auto const maxPerRoute = 10;

    size_t numDestroyed = 0;
    for (auto const neighbour : neighbours)
    {
        auto *U = &context.nodes[neighbour];
        if (!U->route())
            continue;

        auto *route = U->route();
        size_t routeDestroyed = 0;  // Count per route

        while (!U->isDepot() && routeDestroyed < maxPerRoute)
        {
            context.promising[U->client()] = true;
            auto next = n(U);
            route->remove(U->idx());
            route->update();
            U = next;

            routeDestroyed++;
            if (++numDestroyed == numPerturb_)
                return;
        }
    }
}

pyvrp::search::StringRemoval::StringRemoval(ProblemData const &data,
                                            size_t const numPerturb)
    : data_(data), numPerturb_(numPerturb)
{
}
