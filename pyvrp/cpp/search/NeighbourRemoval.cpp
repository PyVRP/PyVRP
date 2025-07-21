#include "NeighbourRemoval.h"

using pyvrp::search::NeighbourRemoval;

void NeighbourRemoval::operator()(PerturbationContext const &context)
{
    if (context.numPerturb == 0 || data_.numClients() == 0)
        return;

    auto const client = context.orderNodes[0];  // random client
    size_t numRemoved = 0;

    for (auto const idx : context.neighbours[client])
    {
        auto *U = &context.nodes[idx];
        if (!U->route())
            continue;

        context.promising[U->client()] = true;
        context.promising[p(U)->client()] = true;
        context.promising[n(U)->client()] = true;

        auto *route = U->route();
        route->remove(U->idx());
        route->update();

        if (++numRemoved == context.numPerturb)
            return;
    }
}

NeighbourRemoval::NeighbourRemoval(ProblemData const &data) : data_(data) {}
