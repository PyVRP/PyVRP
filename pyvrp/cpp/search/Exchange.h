#ifndef PYVRP_EXCHANGE_H
#define PYVRP_EXCHANGE_H

#include "LocalSearchOperator.h"
#include "TimeWindowSegment.h"

#include <cassert>

using TWS = TimeWindowSegment;

/**
 * Template class that exchanges N consecutive nodes from U's route (starting at
 * U) with M consecutive nodes from V's route (starting at V). As special cases,
 * (1, 0) is pure relocate, and (1, 1) pure swap.
 */
template <size_t N, size_t M> class Exchange : public LocalSearchOperator<Node>
{
    using LocalSearchOperator::LocalSearchOperator;

    static_assert(N >= M && N > 0, "N < M or N == 0 does not make sense");

    // Tests if the segment starting at node of given length contains the depot
    inline bool containsDepot(Node *node, size_t segLength) const;

    // Tests if the segments of U and V overlap in the same route
    inline bool overlap(Node *U, Node *V) const;

    // Tests if the segments of U and V are adjacent in the same route
    inline bool adjacent(Node *U, Node *V) const;

    // Special case that's applied when M == 0
    Cost evalRelocateMove(Node *U,
                          Node *V,
                          CostEvaluator const &costEvaluator) const;

    // Applied when M != 0
    Cost
    evalSwapMove(Node *U, Node *V, CostEvaluator const &costEvaluator) const;

public:
    Cost
    evaluate(Node *U, Node *V, CostEvaluator const &costEvaluator) override;

    void apply(Node *U, Node *V) const override;
};

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
    return U->route == V->route
           // We need max(M, 1) here because when V is the depot and M == 0,
           // this would turn negative and wrap around to a large number.
           && U->position <= V->position + std::max<size_t>(M, 1) - 1
           && V->position <= U->position + N - 1;
}

template <size_t N, size_t M>
bool Exchange<N, M>::adjacent(Node *U, Node *V) const
{
    if (U->route != V->route)
        return false;

    return U->position + N == V->position || V->position + M == U->position;
}

template <size_t N, size_t M>
Cost Exchange<N, M>::evalRelocateMove(Node *U,
                                      Node *V,
                                      CostEvaluator const &costEvaluator) const
{
    auto const posU = U->position;
    auto const posV = V->position;

    assert(posU > 0);
    auto *endU = N == 1 ? U : (*U->route)[posU + N - 1];

    auto const deltaDistU = data.dist(p(U)->client, n(endU)->client)
                            - U->route->distBetween(posU - 1, posU + N);
    auto const deltaDistV = data.dist(V->client, U->client)
                            + U->route->distBetween(posU, posU + N - 1)
                            + data.dist(endU->client, n(V)->client)
                            - data.dist(V->client, n(V)->client);

    if (U->route != V->route)
    {
        auto const currentCost = U->route->penalisedCost(costEvaluator)
                                 + V->route->penalisedCost(costEvaluator);

        auto const &vehicleTypeU = data.vehicleType(U->route->vehicleType());
        auto const &vehicleTypeV = data.vehicleType(V->route->vehicleType());

        // Compute lower bound for new cost based on distance and load
        auto const distU = U->route->dist() + deltaDistU;
        auto const distV = V->route->dist() + deltaDistV;

        auto const deltaLoad = U->route->loadBetween(posU, posU + N - 1);
        auto const loadU = U->route->load() - deltaLoad;
        auto const loadV = V->route->load() + deltaLoad;

        auto const lbCostU = costEvaluator.penalisedRouteCost(
            distU, loadU, 0, 0, vehicleTypeU);
        auto const lbCostV = costEvaluator.penalisedRouteCost(
            distV, loadV, 0, 0, vehicleTypeV);

        if (lbCostU + lbCostV >= currentCost)
            return 0;

        // Add time warp for route U to get actual cost
        auto uTWS = TWS::merge(
            data.durationMatrix(), p(U)->twBefore, n(endU)->twAfter);
        auto const costU = costEvaluator.penalisedRouteCost(
            distU, loadU, uTWS, vehicleTypeU);

        // Small optimization, check intermediate bound
        if (costU + lbCostV >= currentCost)
            return 0;

        // Add timing information for route V to get actual cost
        auto vTWS = TWS::merge(data.durationMatrix(),
                               V->twBefore,
                               U->route->twBetween(posU, posU + N - 1),
                               n(V)->twAfter);
        auto const costV = costEvaluator.penalisedRouteCost(
            distV, loadV, vTWS, vehicleTypeV);

        return costU + costV - currentCost;
    }
    else  // within same route
    {
        auto const *route = U->route;
        auto const &vehicleType = data.vehicleType(route->vehicleType());
        auto const currentCost = route->penalisedCost(costEvaluator);
        auto const dist = route->dist() + deltaDistU + deltaDistV;

        // First compute bound based on dist and load
        auto const lbCost = costEvaluator.penalisedRouteCost(
            dist, route->load(), 0, 0, vehicleType);
        if (lbCost >= currentCost)
            return 0;

        // Add timing information for route to get actual cost
        auto const tws = (posU < posV)
                             ? TWS::merge(data.durationMatrix(),
                                          p(U)->twBefore,
                                          route->twBetween(posU + N, posV),
                                          route->twBetween(posU, posU + N - 1),
                                          n(V)->twAfter)
                             : TWS::merge(data.durationMatrix(),
                                          V->twBefore,
                                          route->twBetween(posU, posU + N - 1),
                                          route->twBetween(posV + 1, posU - 1),
                                          n(endU)->twAfter);

        auto const cost = costEvaluator.penalisedRouteCost(
            dist, route->load(), tws, vehicleType);
        return cost - currentCost;
    }
}

template <size_t N, size_t M>
Cost Exchange<N, M>::evalSwapMove(Node *U,
                                  Node *V,
                                  CostEvaluator const &costEvaluator) const
{
    auto const posU = U->position;
    auto const posV = V->position;

    assert(posU > 0 && posV > 0);
    auto *endU = N == 1 ? U : (*U->route)[posU + N - 1];
    auto *endV = M == 1 ? V : (*V->route)[posV + M - 1];

    assert(U->route && V->route);

    //   p(U) -> V -> ... -> endV -> n(endU)
    // - p(U) -> ... -> n(endU)
    auto const deltaDistU = data.dist(p(U)->client, V->client)
                            + V->route->distBetween(posV, posV + M - 1)
                            + data.dist(endV->client, n(endU)->client)
                            - U->route->distBetween(posU - 1, posU + N);

    // + p(V) -> U -> ... -> endU -> n(endV)
    // - p(V) -> ... -> n(endV)
    auto const deltaDistV = data.dist(p(V)->client, U->client)
                            + U->route->distBetween(posU, posU + N - 1)
                            + data.dist(endU->client, n(endV)->client)
                            - V->route->distBetween(posV - 1, posV + M);

    if (U->route != V->route)
    {
        auto const currentCost = U->route->penalisedCost(costEvaluator)
                                 + V->route->penalisedCost(costEvaluator);

        auto const &vehicleTypeU = data.vehicleType(U->route->vehicleType());
        auto const &vehicleTypeV = data.vehicleType(V->route->vehicleType());

        // Compute lower bound for new cost based on distance and load
        auto const distU = U->route->dist() + deltaDistU;
        auto const distV = V->route->dist() + deltaDistV;

        auto const deltaLoad = U->route->loadBetween(posU, posU + N - 1)
                               - V->route->loadBetween(posV, posV + M - 1);
        auto const loadU = U->route->load() - deltaLoad;
        auto const loadV = V->route->load() + deltaLoad;

        auto const lbCostU = costEvaluator.penalisedRouteCost(
            distU, loadU, 0, 0, vehicleTypeU);
        auto const lbCostV = costEvaluator.penalisedRouteCost(
            distV, loadV, 0, 0, vehicleTypeV);

        if (lbCostU + lbCostV >= currentCost)
            return 0;

        // Add timing information for route U to get actual cost
        auto uTWS = TWS::merge(data.durationMatrix(),
                               p(U)->twBefore,
                               V->route->twBetween(posV, posV + M - 1),
                               n(endU)->twAfter);
        auto const costU = costEvaluator.penalisedRouteCost(
            distU, loadU, uTWS, vehicleTypeU);

        if (costU + lbCostV >= currentCost)
            return 0;

        // Add timing information for route V to get actual cost
        auto vTWS = TWS::merge(data.durationMatrix(),
                               p(V)->twBefore,
                               U->route->twBetween(posU, posU + N - 1),
                               n(endV)->twAfter);
        auto const costV = costEvaluator.penalisedRouteCost(
            distV, loadV, vTWS, vehicleTypeV);

        return costU + costV - currentCost;
    }
    else  // within same route
    {
        auto const *route = U->route;
        auto const &vehicleType = data.vehicleType(route->vehicleType());
        auto const currentCost = route->penalisedCost(costEvaluator);
        auto const dist = route->dist() + deltaDistU + deltaDistV;

        // First compute bound based on dist and load
        auto const lbCost = costEvaluator.penalisedRouteCost(
            dist, route->load(), 0, 0, vehicleType);
        if (lbCost >= currentCost)
            return 0;

        auto const tws = (posU < posV)
                             ? TWS::merge(data.durationMatrix(),
                                          p(U)->twBefore,
                                          route->twBetween(posV, posV + M - 1),
                                          route->twBetween(posU + N, posV - 1),
                                          route->twBetween(posU, posU + N - 1),
                                          n(endV)->twAfter)
                             : TWS::merge(data.durationMatrix(),
                                          p(V)->twBefore,
                                          route->twBetween(posU, posU + N - 1),
                                          route->twBetween(posV + M, posU - 1),
                                          route->twBetween(posV, posV + M - 1),
                                          n(endU)->twAfter);

        auto const cost = costEvaluator.penalisedRouteCost(
            dist, route->load(), tws, vehicleType);
        return cost - currentCost;
    }
}

template <size_t N, size_t M>
Cost Exchange<N, M>::evaluate(Node *U,
                              Node *V,
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

template <size_t N, size_t M> void Exchange<N, M>::apply(Node *U, Node *V) const
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

#endif  // PYVRP_EXCHANGE_H
