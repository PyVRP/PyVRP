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
                        + d_params.dist(V->client, n(V)->client);

    int const proposed = d_params.dist(V->client, U->client)
                         + U->route->distBetween(posU, posU + N - 1)
                         + d_params.dist(endU->client, n(V)->client)
                         + d_params.dist(p(U)->client, n(endU)->client);

    int deltaCost = proposed - current;

    if (U->route != V->route)
    {
        if (U->route->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uTWS = TWS::merge(p(U)->twBefore, n(endU)->twAfter);

        deltaCost += d_params.twPenalty(uTWS.totalTimeWarp());
        deltaCost -= d_params.twPenalty(U->route->timeWarp());

        auto const loadDiff = U->route->loadBetween(posU, posU + N - 1);

        deltaCost += d_params.loadPenalty(U->route->load() - loadDiff);
        deltaCost -= d_params.loadPenalty(U->route->load());

        if (deltaCost >= 0)    // if delta cost of just U's route is not enough
            return deltaCost;  // even without V, the move will never be good

        deltaCost += d_params.loadPenalty(V->route->load() + loadDiff);
        deltaCost -= d_params.loadPenalty(V->route->load());

        auto vTWS = TWS::merge(V->twBefore,
                               U->route->twBetween(posU, posU + N - 1),
                               n(V)->twAfter);

        deltaCost += d_params.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= d_params.twPenalty(V->route->timeWarp());
    }
    else  // within same route
    {
        auto const *route = U->route;

        if (!route->hasTimeWarp() && deltaCost >= 0)
            return deltaCost;

        if (posU < posV)
        {
            auto const tws = TWS::merge(p(U)->twBefore,
                                        route->twBetween(posU + N, posV),
                                        route->twBetween(posU, posU + N - 1),
                                        n(V)->twAfter);

            deltaCost += d_params.twPenalty(tws.totalTimeWarp());
        }
        else
        {
            auto const tws = TWS::merge(V->twBefore,
                                        route->twBetween(posU, posU + N - 1),
                                        route->twBetween(posV + 1, posU - 1),
                                        n(endU)->twAfter);

            deltaCost += d_params.twPenalty(tws.totalTimeWarp());
        }

        deltaCost -= d_params.twPenalty(route->timeWarp());
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
        = d_params.dist(p(U)->client, V->client)
          + V->route->distBetween(posV, posV + M - 1)
          + d_params.dist(endV->client, n(endU)->client)
          + d_params.dist(p(V)->client, U->client)
          + U->route->distBetween(posU, posU + N - 1)
          + d_params.dist(endU->client, n(endV)->client);

    int deltaCost = proposed - current;

    if (U->route != V->route)
    {
        if (U->route->isFeasible() && V->route->isFeasible() && deltaCost >= 0)
            return deltaCost;

        auto uTWS = TWS::merge(p(U)->twBefore,
                               V->route->twBetween(posV, posV + M - 1),
                               n(endU)->twAfter);

        deltaCost += d_params.twPenalty(uTWS.totalTimeWarp());
        deltaCost -= d_params.twPenalty(U->route->timeWarp());

        auto vTWS = TWS::merge(p(V)->twBefore,
                               U->route->twBetween(posU, posU + N - 1),
                               n(endV)->twAfter);

        deltaCost += d_params.twPenalty(vTWS.totalTimeWarp());
        deltaCost -= d_params.twPenalty(V->route->timeWarp());

        auto const loadU = U->route->loadBetween(posU, posU + N - 1);
        auto const loadV = V->route->loadBetween(posV, posV + M - 1);
        auto const loadDiff = loadU - loadV;

        deltaCost += d_params.loadPenalty(U->route->load() - loadDiff);
        deltaCost -= d_params.loadPenalty(U->route->load());

        deltaCost += d_params.loadPenalty(V->route->load() + loadDiff);
        deltaCost -= d_params.loadPenalty(V->route->load());
    }
    else  // within same route
    {
        auto const *route = U->route;

        if (!route->hasTimeWarp() && deltaCost >= 0)
            return deltaCost;

        if (posU < posV)
        {
            auto const tws = TWS::merge(p(U)->twBefore,
                                        route->twBetween(posV, posV + M - 1),
                                        route->twBetween(posU + N, posV - 1),
                                        route->twBetween(posU, posU + N - 1),
                                        n(endV)->twAfter);

            deltaCost += d_params.twPenalty(tws.totalTimeWarp());
        }
        else
        {
            auto const tws = TWS::merge(p(V)->twBefore,
                                        route->twBetween(posU, posU + N - 1),
                                        route->twBetween(posV + M, posU - 1),
                                        route->twBetween(posV, posV + M - 1),
                                        n(endU)->twAfter);

            deltaCost += d_params.twPenalty(tws.totalTimeWarp());
        }

        deltaCost -= d_params.twPenalty(U->route->timeWarp());
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
    for (size_t count = 0; count != std::min(N, M); ++count)
    {
        U->swapWith(V);
        U = n(U);
        V = n(V);
    }
}

// Explicit instantiations of the few moves we *might* want to have
template class Exchange<1, 0>;
template class Exchange<2, 0>;
template class Exchange<3, 0>;
template class Exchange<1, 1>;
template class Exchange<2, 1>;
template class Exchange<3, 1>;
template class Exchange<2, 2>;
template class Exchange<3, 2>;
template class Exchange<3, 3>;
