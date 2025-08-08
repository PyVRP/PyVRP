#include "RemoveNeighbours.h"

using pyvrp::search::RemoveNeighbours;

void RemoveNeighbours::operator()(PerturbationContext const &context)
{
    if (context.numPerturbations == 0 || data_.numClients() == 0)
        return;

    auto const center = context.orderNodes.front();  // random client
    size_t numRemoved = 0;

    for (auto const neighbour : context.neighbours[center])
    {
        auto *U = &context.nodes[neighbour];

        if (!U->route())
            continue;

        auto *route = U->route();

        for (auto *node : {U, n(U), p(U)})
        {
            if (node->isDepot())
                continue;

            context.promising[p(node)->client()] = true;
            context.promising[node->client()] = true;
            context.promising[n(node)->client()] = true;

            route->remove(node->idx());

            if (++numRemoved == context.numPerturbations)
            {
                route->update();
                return;
            }
        }

        route->update();
    }
}

RemoveNeighbours::RemoveNeighbours(ProblemData const &data) : data_(data) {}
