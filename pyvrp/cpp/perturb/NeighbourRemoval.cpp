#include "NeighbourRemoval.h"

void pyvrp::perturb::NeighbourRemoval::operator()(
    std::vector<search::Route::Node> &nodes,
    [[maybe_unused]] std::vector<search::Route> &routes,
    [[maybe_unused]] CostEvaluator const &costEvaluator,
    std::vector<std::vector<size_t>> const &neighbours,
    RandomNumberGenerator &rng)
{
    auto const client = rng.randint(data.numClients()) + data.numDepots();
    auto const &clientNeighbours = neighbours[client];
    auto const maxDestroy = std::min(numRemovals, clientNeighbours.size());

    for (size_t idx = 0; idx != maxDestroy; ++idx)
    {
        auto *U = &nodes[clientNeighbours[idx]];
        if (!U->route())
            continue;

        auto *route = U->route();
        route->remove(U->idx());
        route->update();
    }
}

pyvrp::perturb::NeighbourRemoval::NeighbourRemoval(ProblemData const &data,
                                                   size_t const numRemovals)
    : data(data), numRemovals(numRemovals)
{
}
