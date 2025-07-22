#include "NeighbourRemoval.h"

using pyvrp::search::NeighbourRemoval;

void NeighbourRemoval::operator()(PerturbationContext const &context)
{
    if (context.numPerturb == 0 || data_.numClients() == 0)
        return;

    auto const center = context.orderNodes.front();  // random
    std::vector<size_t> nodes{center};
    for (auto const neighbour : context.neighbours[center])
        nodes.push_back(neighbour);

    size_t numRemoved = 0;

    for (auto const idx : nodes)
    {
        auto *U = &context.nodes[idx];

        if (!U->route())
            continue;

        for (auto *node : {U, n(U), p(U)})
        {
            if (node->isStartDepot() || node->isEndDepot())
                continue;

            context.promising[node->client()] = true;
            context.promising[p(node)->client()] = true;
            context.promising[n(node)->client()] = true;

            auto *route = node->route();
            route->remove(node->idx());
            route->update();

            if (++numRemoved == context.numPerturb)
                return;
        }
    }
}

NeighbourRemoval::NeighbourRemoval(ProblemData const &data) : data_(data) {}
