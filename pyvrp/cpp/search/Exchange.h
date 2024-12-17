#ifndef PYVRP_SEARCH_EXCHANGE_H
#define PYVRP_SEARCH_EXCHANGE_H

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
    // Segment exceeds route size, so it must contain a depot.
    if (node->idx() + segLength >= node->route()->size())
        return true;

    return node->route()->containsDepot(node->idx(), segLength);
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

        // We're going to incur V's fixed cost if V is currently empty.
        if (V->idx() == 0 && vRoute->empty())
            deltaCost += vRoute->fixedVehicleCost();

        // We lose U's fixed cost if we're moving all U's clients.
        if (uRoute->numClients() == N)
            deltaCost -= uRoute->fixedVehicleCost();

        auto const uProposal = uRoute->proposal(uRoute->before(U->idx() - 1),
                                                uRoute->after(U->idx() + N));

        if (!V->isDepotUnload())
        {
            auto const vProposal
                = vRoute->proposal(vRoute->before(V->idx()),
                                   uRoute->between(U->idx(), U->idx() + N - 1),
                                   vRoute->after(V->idx() + 1));

            costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
        }
        else if (V->idx() < vRoute->size() - 1)  // new trip in middle of route
        {
            auto const vProposal
                = vRoute->proposal(vRoute->before(V->idx()),
                                   vRoute->startDepotSegment(),
                                   uRoute->between(U->idx(), U->idx() + N - 1),
                                   vRoute->endDepotSegment(),
                                   vRoute->after(V->idx() + 1));

            costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
        }
        else  // new trip at end of route
        {
            auto const vProposal
                = vRoute->proposal(vRoute->before(V->idx()),
                                   vRoute->startDepotSegment(),
                                   uRoute->between(U->idx(), U->idx() + N - 1),
                                   vRoute->endDepotSegment());

            costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
        }
    }
    else  // within same route
    {
        auto *route = U->route();

        if (!V->isDepotUnload())
        {
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
        else if (V->idx() < route->size() - 1)  // new trip in middle of route.
        {
            if (U->idx() < V->idx())
                costEvaluator.deltaCost(
                    deltaCost,
                    route->proposal(route->before(U->idx() - 1),
                                    route->between(U->idx() + N, V->idx()),
                                    route->startDepotSegment(),
                                    route->between(U->idx(), U->idx() + N - 1),
                                    route->endDepotSegment(),
                                    route->after(V->idx() + 1)));
            else
                costEvaluator.deltaCost(
                    deltaCost,
                    route->proposal(route->before(V->idx()),
                                    route->startDepotSegment(),
                                    route->between(U->idx(), U->idx() + N - 1),
                                    route->endDepotSegment(),
                                    route->between(V->idx() + 1, U->idx() - 1),
                                    route->after(U->idx() + N)));
        }
        else  // new trip at end of route.
        {
            assert(U->idx() < V->idx());
            costEvaluator.deltaCost(
                deltaCost,
                route->proposal(route->before(U->idx() - 1),
                                route->between(U->idx() + N, V->idx()),
                                route->startDepotSegment(),
                                route->between(U->idx(), U->idx() + N - 1),
                                route->endDepotSegment()));
        }
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
        if (!V->isDepotUnload() && U == n(V))
            return 0;

        // Cannot exceed max trips.
        if (V->isDepotUnload()
            && V->route()->numTrips() == V->route()->maxTrips())
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

    // If inserting after a depot unload node, then a new trip is created.
    if (insertUAfter->isDepotUnload())
    {
        vRoute.insertTrip(insertUAfter->idx() + 1);
        insertUAfter = n(insertUAfter);
    }

    // Insert these 'extra' nodes of U after the end of V...
    for (size_t count = 0; count != N - M; ++count)
    {
        auto *prev = p(uToInsert);
        uRoute.remove(uToInsert->idx());
        vRoute.insert(insertUAfter->idx() + 1, uToInsert);
        uToInsert = prev;
    }

    // Remove depot nodes if empty trip is left in uRoute.
    if (uRoute.numTrips() > 1 && uToInsert->isDepotLoad()
        && n(uToInsert)->isDepotUnload())
        uRoute.removeTrip(uToInsert->tripIdx());

    // ...and swap the overlapping nodes!
    for (size_t count = 0; count != M; ++count)
    {
        Route::swap(U, V);
        U = n(U);
        V = n(V);
    }
}
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_EXCHANGE_H
