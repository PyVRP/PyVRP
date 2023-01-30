#include "Exchange.h"

#include "Route.h"
#include "TimeWindowSegment.h"

using TWS = TimeWindowSegment;

template <size_t N, size_t M>
bool Exchange<N, M>::containsDepot(Node *node, size_t segLength) const
{
    if (node->isDepot())
        return true;

    // size() is the position of the last node in the route. So the segment
    // must include the depot if position + move length - 1 (-1 since we're
    // also moving the node *at* position) is larger than size().
    return node->position + segLength - 1 > node->route->size();
}

template <size_t N, size_t M>
bool Exchange<N, M>::overlap(Node *U, Node *V) const
{
    // clang-format off
    return U->route == V->route
        && U->position <= V->position + M - 1
        && V->position <= U->position + N - 1;
    // clang-format on
}

template <size_t N, size_t M>
bool Exchange<N, M>::adjacent(Node *U, Node *V) const
{
    if (U->route != V->route)
        return false;

    return U->position + N == V->position || V->position + M == U->position;
}

template <size_t N, size_t M>
int Exchange<N, M>::evalRelocateMove(Node *U, Node *V) const
{
    auto *endU = N == 1 ? U : (*U->route)[U->position + N - 1];
    auto const posU = U->position;
    auto const posV = V->position;

    int const current = U->route->distBetween(posU - 1, posU + N)
                        + data.dist(V->client, n(V)->client);

    int const proposed = data.dist(V->client, U->client)
                         + U->route->distBetween(posU, posU + N - 1)
                         + data.dist(endU->client, n(V)->client)
                         + data.dist(p(U)->client, n(endU)->client);

    int deltaCost = proposed - current;

    if (U->route != V->route)
    {
        // If the route is feasible then deltaCost is a lower bound
        // on the overall deltaCost after accounting for penalties
        // TODO: is this check worth it? If the penalty computations
        // until the next precheck are fast enough we can better skip it
        if (U->route->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto const loadDiff = U->route->loadBetween(posU, posU + N - 1);

        // <= 0
        deltaCost += penaltyManager.loadPenalty(U->route->load() - loadDiff);
        deltaCost -= penaltyManager.loadPenalty(U->route->load());

        // >= 0
        deltaCost += penaltyManager.loadPenalty(V->route->load() + loadDiff);
        deltaCost -= penaltyManager.loadPenalty(V->route->load());

        // In the best case, the timeWarp in route U is completely resolved
        // and the timeWarp in route V does not increase, so check that first
        // important: this relies on the assumption that timewarp will never
        // decrease by inserting a customer (this requires the triangle
        // inequality to hold)
        // <= 0
        deltaCost -= penaltyManager.twPenalty(U->route->timeWarp());

        // Now the deltaCost accounts for distance, load penalties and
        // assumes we completely resolve the timewarp in U. It does not
        // count the remaining timewarp in U and increase in timewarp in
        // V and therefore is a lower bound.
        if (deltaCost >= 0)
            return deltaCost;

        // Find the actual reduction in timeWarp penalty
        auto uTWS = TWS::merge(p(U)->twBefore, n(endU)->twAfter);
        deltaCost += penaltyManager.twPenalty(uTWS.totalTimeWarp());

        // Now the deltaCost accounts for everything except the increase
        // in timeWarp in route V which is >= 0, so this is a lower bound.
        if (deltaCost >= 0)
            return deltaCost;

        auto vTWS = TWS::merge(V->twBefore,
                               U->route->twBetween(posU, posU + N - 1),
                               n(V)->twAfter);

        // Since we insert into route V, total timeWarp should not decrease
        // (assuming travel times satisfy triangle inequality)
        // so deltaCost changes by >= 0
        deltaCost += penaltyManager.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= penaltyManager.twPenalty(V->route->timeWarp());
    }
    else  // within same route
    {
        auto const *route = U->route;

        // TODO: is this check worth it? If penaltyManager.twPenalty
        // is fast enough we can skip it
        if (!route->hasTimeWarp() && deltaCost >= 0)
            return deltaCost;

        deltaCost -= penaltyManager.twPenalty(route->timeWarp());

        // Now the deltaCost assumes we completely resolve timeWarp so this
        // is a lower bound
        if (deltaCost >= 0)
            return deltaCost;

        if (posU < posV)
        {
            auto const tws = TWS::merge(p(U)->twBefore,
                                        route->twBetween(posU + N, posV),
                                        route->twBetween(posU, posU + N - 1),
                                        n(V)->twAfter);

            deltaCost += penaltyManager.twPenalty(tws.totalTimeWarp());
        }
        else
        {
            auto const tws = TWS::merge(V->twBefore,
                                        route->twBetween(posU, posU + N - 1),
                                        route->twBetween(posV + 1, posU - 1),
                                        n(endU)->twAfter);

            deltaCost += penaltyManager.twPenalty(tws.totalTimeWarp());
        }
    }

    return deltaCost;
}

template <size_t N, size_t M>
int Exchange<N, M>::evalSwapMove(Node *U, Node *V) const
{
    auto *endU = N == 1 ? U : (*U->route)[U->position + N - 1];
    auto *endV = M == 1 ? V : (*V->route)[V->position + M - 1];

    auto const posU = U->position;
    auto const posV = V->position;

    int const current = U->route->distBetween(posU - 1, posU + N)
                        + V->route->distBetween(posV - 1, posV + M);

    int const proposed
        //   p(U) -> V -> ... -> endV -> n(endU)
        // + p(V) -> U -> ... -> endU -> n(endV)
        = data.dist(p(U)->client, V->client)
          + V->route->distBetween(posV, posV + M - 1)
          + data.dist(endV->client, n(endU)->client)
          + data.dist(p(V)->client, U->client)
          + U->route->distBetween(posU, posU + N - 1)
          + data.dist(endU->client, n(endV)->client);

    int deltaCost = proposed - current;

    if (U->route != V->route)
    {
        // TODO is this precheck still worth it?
        if (U->route->isFeasible() && V->route->isFeasible() && deltaCost >= 0)
            return deltaCost;

        // Compute deltaCost for load penalties
        auto const loadU = U->route->loadBetween(posU, posU + N - 1);
        auto const loadV = V->route->loadBetween(posV, posV + M - 1);
        auto const loadDiff = loadU - loadV;

        deltaCost += penaltyManager.loadPenalty(U->route->load() - loadDiff);
        deltaCost -= penaltyManager.loadPenalty(U->route->load());

        deltaCost += penaltyManager.loadPenalty(V->route->load() + loadDiff);
        deltaCost -= penaltyManager.loadPenalty(V->route->load());

        // Compute deltaCost assuming all time warp will disappear
        // so we can use it to compute a bound
        deltaCost -= penaltyManager.twPenalty(U->route->timeWarp());
        deltaCost -= penaltyManager.twPenalty(V->route->timeWarp());

        // Now deltaCost is a lowerBound
        if (deltaCost >= 0)
            return deltaCost;

        // Since N >= M, the number of nodes in route V will increase
        // so route V is more likely to have a (large) timewarp
        // so compute this first
        // (on the other hand it is also more expensive to compute... TODO)
        auto vTWS = TWS::merge(p(V)->twBefore,
                               U->route->twBetween(posU, posU + N - 1),
                               n(endV)->twAfter);

        deltaCost += penaltyManager.twPenalty(vTWS.totalTimeWarp());

        // Now we included the new timewarp penalty for V but by disregarding
        // U we still have a lower bound and we may skip checking U
        if (deltaCost >= 0)
            return deltaCost;

        // Now add back the actual timewarp for U to get final delta cost

        auto uTWS = TWS::merge(p(U)->twBefore,
                               V->route->twBetween(posV, posV + M - 1),
                               n(endU)->twAfter);

        deltaCost += penaltyManager.twPenalty(uTWS.totalTimeWarp());
        
        
    }
    else  // within same route
    {
        auto const *route = U->route;

        // TODO: is this check worth it? If penaltyManager.twPenalty
        // is fast enough we can skip it
        if (!route->hasTimeWarp() && deltaCost >= 0)
            return deltaCost;

        deltaCost -= penaltyManager.twPenalty(route->timeWarp());

        // Now the deltaCost assumes we completely resolve timeWarp so this
        // is a lower bound
        if (deltaCost >= 0)
            return deltaCost;


        if (posU < posV)
        {
            auto const tws = TWS::merge(p(U)->twBefore,
                                        route->twBetween(posV, posV + M - 1),
                                        route->twBetween(posU + N, posV - 1),
                                        route->twBetween(posU, posU + N - 1),
                                        n(endV)->twAfter);

            deltaCost += penaltyManager.twPenalty(tws.totalTimeWarp());
        }
        else
        {
            auto const tws = TWS::merge(p(V)->twBefore,
                                        route->twBetween(posU, posU + N - 1),
                                        route->twBetween(posV + M, posU - 1),
                                        route->twBetween(posV, posV + M - 1),
                                        n(endU)->twAfter);

            deltaCost += penaltyManager.twPenalty(tws.totalTimeWarp());
        }
    }

    return deltaCost;
}

template <size_t N, size_t M> int Exchange<N, M>::evaluate(Node *U, Node *V)
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

        return evalRelocateMove(U, V);
    }
    else
    {
        if constexpr (N == M)  // symmetric, so only have to evaluate this once
            if (U->client >= V->client)
                return 0;

        if (adjacent(U, V))
            return 0;

        return evalSwapMove(U, V);
    }
}

template <size_t N, size_t M> void Exchange<N, M>::apply(Node *U, Node *V)
{
    auto *uToInsert = N == 1 ? U : (*U->route)[U->position + N - 1];
    auto *insertUAfter = M == 0 ? V : (*V->route)[V->position + M - 1];

    // Insert these 'extra' nodes of U after the end of V...
    for (size_t count = 0; count != N - M; ++count)
    {
        auto *prev = p(uToInsert);
        uToInsert->insertAfter(insertUAfter);
        uToInsert = prev;
    }

    // ...and swap the overlapping nodes!
    for (size_t count = 0; count != M; ++count)
    {
        U->swapWith(V);
        U = n(U);
        V = n(V);
    }
}

// Explicit instantiations of the few moves we *might* want to have
// TODO get rid of this
template class Exchange<1, 0>;
template class Exchange<2, 0>;
template class Exchange<3, 0>;
template class Exchange<1, 1>;
template class Exchange<2, 1>;
template class Exchange<3, 1>;
template class Exchange<2, 2>;
template class Exchange<3, 2>;
template class Exchange<3, 3>;
