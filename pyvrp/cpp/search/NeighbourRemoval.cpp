#include "NeighbourRemoval.h"

void pyvrp::search::NeighbourRemoval::operator()(
    std::vector<search::Route::Node> &nodes,
    [[maybe_unused]] std::vector<search::Route> &routes,
    [[maybe_unused]] CostEvaluator const &costEvaluator,
    std::vector<std::vector<size_t>> const &neighbours,
    std::vector<size_t> const &orderNodes)
{
    if (numRemovals == 0 || data.numClients() == 0)
        return;

    auto const client = orderNodes[0];  // random client
    size_t numRemoved = 0;

    for (auto const idx : neighbours[client])
    {
        auto *U = &nodes[idx];
        if (!U->route())
            continue;

        auto *route = U->route();
        route->remove(U->idx());
        route->update();

        if (++numRemoved == numRemovals)
            return;
    }
}

pyvrp::search::NeighbourRemoval::NeighbourRemoval(ProblemData const &data,
                                                  size_t const numRemovals)
    : data(data), numRemovals(numRemovals)
{
}
