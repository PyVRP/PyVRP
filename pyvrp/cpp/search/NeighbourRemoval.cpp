#include "NeighbourRemoval.h"

void pyvrp::search::NeighbourRemoval::operator()(
    std::vector<search::Route::Node> &nodes,
    [[maybe_unused]] std::vector<search::Route> &routes,
    [[maybe_unused]] CostEvaluator const &costEvaluator,
    std::vector<std::vector<size_t>> const &neighbours,
    std::vector<size_t> const &orderNodes)
{
    auto const client = orderNodes[0];  // random client
    auto const &neighbourhood = neighbours[client];
    auto const maxDestroy = std::min(numDestroy, neighbourhood.size());

    for (size_t idx = 0; idx != maxDestroy; ++idx)
    {
        auto *U = &nodes[neighbourhood[idx]];
        if (!U->route())
            continue;

        auto *route = U->route();
        route->remove(U->idx());
        route->update();
    }
}

pyvrp::search::NeighbourRemoval::NeighbourRemoval(ProblemData const &data,
                                                  size_t const numDestroy)
    : data(data), numDestroy(numDestroy)
{
}
