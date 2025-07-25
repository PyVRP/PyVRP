#ifndef PYVRP_SEARCH_SWAPINPLACE
#define PYVRP_SEARCH_SWAPINPLACE

#include "LocalSearchOperator.h"
#include "Route.h"
#include "primitives.h"

namespace pyvrp::search
{
/**
 * Simple wrapper class that implements the required evaluation interface for
 * a single client that might not currently be in the solution.
 */
class ClientSegment
{
    ProblemData const &data;
    size_t client;

public:
    ClientSegment(ProblemData const &data, size_t client)
        : data(data), client(client)
    {
        assert(client >= data.numDepots());  // must be an actual client
    }

    pyvrp::search::Route const *route() const { return nullptr; }

    size_t first() const { return client; }
    size_t last() const { return client; }
    size_t size() const { return 1; }

    bool startsAtReloadDepot() const { return false; }
    bool endsAtReloadDepot() const { return false; }

    pyvrp::Distance distance([[maybe_unused]] size_t profile) const
    {
        return 0;
    }

    pyvrp::DurationSegment duration([[maybe_unused]] size_t profile) const
    {
        ProblemData::Client const &clientData = data.location(client);
        return {clientData};
    }

    pyvrp::LoadSegment load(size_t dimension) const
    {
        return {data.location(client), dimension};
    }
};
/**
 * SwapInPlace(data: ProblemData)
 *
 * Given two optional nodes :math:`U` and :math:`V`, where :math:`U` is in a
 * route :math:`V` is not, this operator tests whether swapping V for U, U+1,
 * ..., U+N is an improving move.
 */
template <size_t N> class SwapInPlace : public NodeOperator
{
    using NodeOperator::NodeOperator;

    // Tests if the segment starting at node of given length contains the depot
    bool containsDepot(Route::Node *node, size_t segLength) const;

public:
    Cost evaluate(Route::Node *U,
                  Route::Node *V,
                  CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};

template <size_t N>
bool SwapInPlace<N>::containsDepot(Route::Node *node, size_t segLength) const
{
    auto const first = node->idx();
    auto const last = first + segLength - 1;
    auto const &route = *node->route();

    return first == 0                               // contains start depot
           || last >= route.size() - 1              // contains end depot
           || node->trip() != route[last]->trip();  // contains reload depot
}

template <size_t N>
Cost SwapInPlace<N>::evaluate(Route::Node *U,
                              Route::Node *V,
                              CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (!U->route() || V->route())
        return 0;

    if (containsDepot(U, N))
        return 0;

    Cost deltaCost = 0;
    auto const *route = U->route();

    ProblemData::Client const &clientData = data.location(V->client());
    deltaCost -= clientData.prize;

    auto *node = U;
    for (size_t idx = 0; idx < N; ++idx)
    {
        ProblemData::Client const &clientData = data.location(node->client());
        deltaCost += clientData.prize;
        node = n(U);
    }

    costEvaluator.deltaCost(deltaCost,
                            Route::Proposal(route->before(U->idx() - 1),
                                            ClientSegment(data, V->client()),
                                            route->after(U->idx() + N)));

    return deltaCost;
}

template <size_t N>
void SwapInPlace<N>::apply(Route::Node *U, Route::Node *V) const
{
    stats_.numApplications++;

    auto &route = *U->route();
    auto const pos = U->idx();  // insert and remove position

    // Remove U, U+1, ..., U+N.
    for (size_t idx = 0; idx < N; ++idx)
        route.remove(pos);

    // Insert V at U.
    route.insert(pos, V);
}

// TODO supports
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SWAPINPLACE
