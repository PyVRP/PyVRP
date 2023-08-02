#ifndef PYVRP_EXCHANGE_H
#define PYVRP_EXCHANGE_H

#include "LocalSearchOperator.h"
#include "TimeWindowSegment.h"

#include <cassert>

namespace pyvrp::search
{
/**
 * Exchange()
 *
 * The :math:`(N, M)`-exchange operators exhange :math:`N` consecutive clients
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
    inline bool containsDepot(Route::Node *node, size_t segLength) const;

    // Tests if the segments of U and V overlap in the same route
    inline bool overlap(Route::Node *U, Route::Node *V) const;

    // Tests if the segments of U and V are adjacent in the same route
    inline bool adjacent(Route::Node *U, Route::Node *V) const;

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
    if (node->isDepot())
        return true;

    // size() is the position of the last node in the route. So the segment
    // must include the depot if position + move length - 1 (-1 since we're
    // also moving the node *at* position) is larger than size().
    return node->position + segLength - 1 > node->route->size();
}

template <size_t N, size_t M>
bool Exchange<N, M>::overlap(Route::Node *U, Route::Node *V) const
{
    return U->route == V->route
           // We need max(M, 1) here because when V is the depot and M == 0,
           // this would turn negative and wrap around to a large number.
           && U->position <= V->position + std::max<size_t>(M, 1) - 1
           && V->position <= U->position + N - 1;
}

template <size_t N, size_t M>
bool Exchange<N, M>::adjacent(Route::Node *U, Route::Node *V) const
{
    if (U->route != V->route)
        return false;

    return U->position + N == V->position || V->position + M == U->position;
}

template <size_t N, size_t M>
Cost Exchange<N, M>::evalRelocateMove(Route::Node *U,
                                      Route::Node *V,
                                      CostEvaluator const &costEvaluator) const
{
    auto const posU = U->position;
    auto const posV = V->position;

    assert(posU > 0);
    auto *endU = N == 1 ? U : (*U->route)[posU + N - 1];

    Distance const current = U->route->distBetween(posU - 1, posU + N)
                             + data.dist(V->client, n(V)->client);

    Distance const proposed = data.dist(V->client, U->client)
                              + U->route->distBetween(posU, posU + N - 1)
                              + data.dist(endU->client, n(V)->client)
                              + data.dist(p(U)->client, n(endU)->client);

    Cost deltaCost = static_cast<Cost>(proposed - current);

    if (U->route != V->route)
    {
        if (U->route->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uTWS = TimeWindowSegment::merge(
            data.durationMatrix(), p(U)->twBefore, n(endU)->twAfter);

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());

        auto const loadDiff = U->route->loadBetween(posU, posU + N - 1);

        deltaCost += costEvaluator.loadPenalty(U->route->load() - loadDiff,
                                               U->route->capacity());
        deltaCost -= costEvaluator.loadPenalty(U->route->load(),
                                               U->route->capacity());

        if (deltaCost >= 0)    // if delta cost of just U's route is not enough
            return deltaCost;  // even without V, the move will never be good.

        deltaCost += costEvaluator.loadPenalty(V->route->load() + loadDiff,
                                               V->route->capacity());
        deltaCost -= costEvaluator.loadPenalty(V->route->load(),
                                               V->route->capacity());

        auto vTWS
            = TimeWindowSegment::merge(data.durationMatrix(),
                                       V->twBefore,
                                       U->route->twBetween(posU, posU + N - 1),
                                       n(V)->twAfter);

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(V->route->timeWarp());
    }
    else  // within same route
    {
        auto const *route = U->route;

        if (!route->hasTimeWarp() && deltaCost >= 0)
            return deltaCost;

        if (posU < posV)
        {
            auto const tws
                = TimeWindowSegment::merge(data.durationMatrix(),
                                           p(U)->twBefore,
                                           route->twBetween(posU + N, posV),
                                           route->twBetween(posU, posU + N - 1),
                                           n(V)->twAfter);

            deltaCost += costEvaluator.twPenalty(tws.totalTimeWarp());
        }
        else
        {
            auto const tws
                = TimeWindowSegment::merge(data.durationMatrix(),
                                           V->twBefore,
                                           route->twBetween(posU, posU + N - 1),
                                           route->twBetween(posV + 1, posU - 1),
                                           n(endU)->twAfter);

            deltaCost += costEvaluator.twPenalty(tws.totalTimeWarp());
        }

        deltaCost -= costEvaluator.twPenalty(route->timeWarp());
    }

    return deltaCost;
}

template <size_t N, size_t M>
Cost Exchange<N, M>::evalSwapMove(Route::Node *U,
                                  Route::Node *V,
                                  CostEvaluator const &costEvaluator) const
{
    auto const posU = U->position;
    auto const posV = V->position;

    assert(posU > 0 && posV > 0);
    auto *endU = N == 1 ? U : (*U->route)[posU + N - 1];
    auto *endV = M == 1 ? V : (*V->route)[posV + M - 1];

    assert(U->route && V->route);

    Distance const current = U->route->distBetween(posU - 1, posU + N)
                             + V->route->distBetween(posV - 1, posV + M);

    Distance const proposed
        //   p(U) -> V -> ... -> endV -> n(endU)
        // + p(V) -> U -> ... -> endU -> n(endV)
        = data.dist(p(U)->client, V->client)
          + V->route->distBetween(posV, posV + M - 1)
          + data.dist(endV->client, n(endU)->client)
          + data.dist(p(V)->client, U->client)
          + U->route->distBetween(posU, posU + N - 1)
          + data.dist(endU->client, n(endV)->client);

    Cost deltaCost = static_cast<Cost>(proposed - current);

    if (U->route != V->route)
    {
        if (U->route->isFeasible() && V->route->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uTWS
            = TimeWindowSegment::merge(data.durationMatrix(),
                                       p(U)->twBefore,
                                       V->route->twBetween(posV, posV + M - 1),
                                       n(endU)->twAfter);

        deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());

        auto const loadU = U->route->loadBetween(posU, posU + N - 1);
        auto const loadV = V->route->loadBetween(posV, posV + M - 1);
        auto const loadDiff = loadU - loadV;

        deltaCost += costEvaluator.loadPenalty(U->route->load() - loadDiff,
                                               U->route->capacity());
        deltaCost -= costEvaluator.loadPenalty(U->route->load(),
                                               U->route->capacity());

        auto vTWS
            = TimeWindowSegment::merge(data.durationMatrix(),
                                       p(V)->twBefore,
                                       U->route->twBetween(posU, posU + N - 1),
                                       n(endV)->twAfter);

        deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= costEvaluator.twPenalty(V->route->timeWarp());

        deltaCost += costEvaluator.loadPenalty(V->route->load() + loadDiff,
                                               V->route->capacity());
        deltaCost -= costEvaluator.loadPenalty(V->route->load(),
                                               V->route->capacity());
    }
    else  // within same route
    {
        auto const *route = U->route;

        if (!route->hasTimeWarp() && deltaCost >= 0)
            return deltaCost;

        if (posU < posV)
        {
            auto const tws
                = TimeWindowSegment::merge(data.durationMatrix(),
                                           p(U)->twBefore,
                                           route->twBetween(posV, posV + M - 1),
                                           route->twBetween(posU + N, posV - 1),
                                           route->twBetween(posU, posU + N - 1),
                                           n(endV)->twAfter);

            deltaCost += costEvaluator.twPenalty(tws.totalTimeWarp());
        }
        else
        {
            auto const tws
                = TimeWindowSegment::merge(data.durationMatrix(),
                                           p(V)->twBefore,
                                           route->twBetween(posU, posU + N - 1),
                                           route->twBetween(posV + M, posU - 1),
                                           route->twBetween(posV, posV + M - 1),
                                           n(endU)->twAfter);

            deltaCost += costEvaluator.twPenalty(tws.totalTimeWarp());
        }

        deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());
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
            if (U->client >= V->client)
                return 0;

        if (adjacent(U, V))
            return 0;

        return evalSwapMove(U, V, costEvaluator);
    }
}

template <size_t N, size_t M>
void Exchange<N, M>::apply(Route::Node *U, Route::Node *V) const
{
    auto &uRoute = *U->route;
    auto &vRoute = *V->route;
    auto *uToInsert = N == 1 ? U : uRoute[U->position + N - 1];
    auto *insertUAfter = M == 0 ? V : vRoute[V->position + M - 1];

    // Insert these 'extra' nodes of U after the end of V...
    for (size_t count = 0; count != N - M; ++count)
    {
        auto *prev = p(uToInsert);
        uRoute.remove(uToInsert->position);
        vRoute.insert(insertUAfter->position + 1, uToInsert);
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
