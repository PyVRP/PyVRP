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

        for (auto *node : {U, n(U), p(U)})
        {
            if (node->isDepot())
                continue;

            context.promising[node->client()] = true;
            context.promising[p(node)->client()] = true;
            context.promising[n(node)->client()] = true;

            auto *route = node->route();
            route->remove(node->idx());
            route->update();

            if (++numRemoved == context.numPerturbations)
                return;
        }
    }
}

RemoveNeighbours::RemoveNeighbours(ProblemData const &data) : data_(data) {}
