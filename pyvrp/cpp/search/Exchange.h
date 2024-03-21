#ifndef PYVRP_EXCHANGE_H
#define PYVRP_EXCHANGE_H

#include "LocalSearchOperator.h"
#include "Route.h"
#include "primitives.h"

#include <cassert>

namespace pyvrp::search
{
/**
 * Exchange(data: ProblemData)
 *
 * The :math:`(N, M)`-exchange operators exchange :math:`N` consecutive clients
 * from :math:`U`'s route (starting at :math:`U`) with :math:`M` consecutive
 * clients from :math:`V`'s route (starting at :math:`V`). This includes
 * the RELOCATE and SWAP operators as special cases.
 *
 * The :math:`(N, M)`-exchange class uses C++ templates for different :math:`N`
 * and :math:`M` to efficiently evaluate these moves.
 */
template <size_t N, size_t M>
class Exchange : public LocalSearchOperator<Route::Node>
{
    using LocalSearchOperator::LocalSearchOperator;

    static_assert(N >= M && N > 0, "N < M or N == 0 does not make sense");

    // Tests if the segment starting at node of given length contains the depot
    bool containsDepot(Route::Node *node, size_t segLength) const;

    // Tests if the segments of U and V overlap in the same route
    bool overlap(Route::Node *U, Route::Node *V) const;

    // Tests if the segments of U and V are adjacent in the same route
    bool adjacent(Route::Node *U, Route::Node *V) const;

    // Special case that's applied when M == 0
    Cost evalRelocateMove(Route::Node *U,
                          Route::Node *V,
                          CostEvaluator const &costEvaluator) const;

    // Applied when M != 0
    Cost evalSwapMove(Route::Node *U,
                      Route::Node *V,
                      CostEvaluator const &costEvaluator) const;

public:
    Cost evaluate(Route::Node *U,
                  Route::Node *V,
                  CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;
};

template <size_t N, size_t M>
bool Exchange<N, M>::containsDepot(Route::Node *node, size_t segLength) const
{
    // size() is the position of the last client in the route. So the segment
    // must include the depot if idx + move length - 1 (-1 since we're also
    // moving the node *at* idx) is larger than size().
    return node->isDepot()
           || (node->idx() + segLength - 1 > node->route()->size());
}

template <size_t N, size_t M>
bool Exchange<N, M>::overlap(Route::Node *U, Route::Node *V) const
{
    return U->route() == V->route()
           // We need max(M, 1) here because when V is the depot and M == 0,
           // this would turn negative and wrap around to a large number.
           && U->idx() <= V->idx() + std::max<size_t>(M, 1) - 1
           && V->idx() <= U->idx() + N - 1;
}

template <size_t N, size_t M>
bool Exchange<N, M>::adjacent(Route::Node *U, Route::Node *V) const
{
    return U->route() == V->route()
           && (U->idx() + N == V->idx() || V->idx() + M == U->idx());
}

template <size_t N, size_t M>
Cost Exchange<N, M>::evalRelocateMove(Route::Node *U,
                                      Route::Node *V,
                                      CostEvaluator const &costEvaluator) const
{
    assert(U->idx() > 0);

    Cost deltaCost = 0;

    if (U->route() != V->route())
    {
        auto const *uRoute = U->route();
        auto const *vRoute = V->route();

        auto const uProposal = uRoute->proposal(uRoute->before(U->idx() - 1),
                                                uRoute->after(U->idx() + N));

        auto const vProposal
            = vRoute->proposal(vRoute->before(V->idx()),
                               uRoute->between(U->idx(), U->idx() + N - 1),
                               vRoute->after(V->idx() + 1));

        // We're going to incur V's fixed cost if V is currently empty. We lose
        // U's fixed cost if we're moving all of U's clients with this operator.
        deltaCost += Cost(vRoute->empty()) * vRoute->fixedVehicleCost();
        deltaCost -= Cost(uRoute->size() == N) * uRoute->fixedVehicleCost();

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else  // within same route
    {
        auto *route = U->route();

        if (U->idx() < V->idx())
            costEvaluator.deltaCost(
                deltaCost,
                route->proposal(route->before(U->idx() - 1),
                                route->between(U->idx() + N, V->idx()),
                                route->between(U->idx(), U->idx() + N - 1),
                                route->after(V->idx() + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                route->proposal(route->before(V->idx()),
                                route->between(U->idx(), U->idx() + N - 1),
                                route->between(V->idx() + 1, U->idx() - 1),
                                route->after(U->idx() + N)));
    }

    return deltaCost;
}

template <size_t N, size_t M>
Cost Exchange<N, M>::evalSwapMove(Route::Node *U,
                                  Route::Node *V,
                                  CostEvaluator const &costEvaluator) const
{
    assert(U->idx() > 0 && V->idx() > 0);
    assert(U->route() && V->route());

    Cost deltaCost = 0;

    if (U->route() != V->route())
    {
        auto const *uRoute = U->route();
        auto const *vRoute = V->route();

        auto const uProposal
            = uRoute->proposal(uRoute->before(U->idx() - 1),
                               vRoute->between(V->idx(), V->idx() + M - 1),
                               uRoute->after(U->idx() + N));

        auto const vProposal
            = vRoute->proposal(vRoute->before(V->idx() - 1),
                               uRoute->between(U->idx(), U->idx() + N - 1),
                               vRoute->after(V->idx() + M));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else  // within same route
    {
        auto const *route = U->route();

        if (U->idx() < V->idx())
            costEvaluator.deltaCost(
                deltaCost,
                route->proposal(route->before(U->idx() - 1),
                                route->between(V->idx(), V->idx() + M - 1),
                                route->between(U->idx() + N, V->idx() - 1),
                                route->between(U->idx(), U->idx() + N - 1),
                                route->after(V->idx() + M)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                route->proposal(route->before(V->idx() - 1),
                                route->between(U->idx(), U->idx() + N - 1),
                                route->between(V->idx() + M, U->idx() - 1),
                                route->between(V->idx(), V->idx() + M - 1),
                                route->after(U->idx() + N)));
    }

    return deltaCost;
}

template <size_t N, size_t M>
Cost Exchange<N, M>::evaluate(Route::Node *U,
                              Route::Node *V,
                              CostEvaluator const &costEvaluator)
{
    if (containsDepot(U, N) || overlap(U, V))
        return 0;

    if constexpr (M > 0)
        if (containsDepot(V, M))
            return 0;

    if constexpr (M == 0)  // special case where nothing in V is moved
    {
        if (U == n(V))
            return 0;

        return evalRelocateMove(U, V, costEvaluator);
    }
    else
    {
        if constexpr (N == M)  // symmetric, so only have to evaluate this once
            if (U->client() >= V->client())
                return 0;

        if (adjacent(U, V))
            return 0;

        return evalSwapMove(U, V, costEvaluator);
    }
}

template <size_t N, size_t M>
void Exchange<N, M>::apply(Route::Node *U, Route::Node *V) const
{
    auto &uRoute = *U->route();
    auto &vRoute = *V->route();
    auto *uToInsert = N == 1 ? U : uRoute[U->idx() + N - 1];
    auto *insertUAfter = M == 0 ? V : vRoute[V->idx() + M - 1];

    // Insert these 'extra' nodes of U after the end of V...
    for (size_t count = 0; count != N - M; ++count)
    {
        auto *prev = p(uToInsert);
        uRoute.remove(uToInsert->idx());
        vRoute.insert(insertUAfter->idx() + 1, uToInsert);
        uToInsert = prev;
    }

    // ...and swap the overlapping nodes!
    for (size_t count = 0; count != M; ++count)
    {
        Route::swap(U, V);
        U = n(U);
        V = n(V);
    }
}
}  // namespace pyvrp::search

#endif  // PYVRP_EXCHANGE_H
