#ifndef PYVRP_SEARCH_SWAP_H
#define PYVRP_SEARCH_SWAP_H

#include "LocalSearchOperator.h"

#include <cassert>

namespace pyvrp::search
{
/**
 * Swap(data: ProblemData)
 *
 * The :math:`(N, M)`-swap operator evaluates swapping :math:`N` consecutive
 * nodes from :math:`U`'s route (starting with :math:`U`) with :math:`M`
 * consecutive nodes from :math:`V`'s route (starting with :math:`V`).
 */
template <size_t N, size_t M> class Swap : public BinaryOperator
{
    using BinaryOperator::BinaryOperator;

    static_assert(N >= M && M > 0, "N < M or M == 0 does not make sense");

    // Tests if the segment starting at node of given length contains the depot.
    bool hasDepot(Route::Node *node, size_t segLength) const;

    // Tests if the segment starting at node of given length would split a
    // shipment if swapped.
    bool splitsShipment(Route::Node *node, size_t segLength) const;

    // Tests if the segments of U and V overlap in the same route.
    bool overlap(Route::Node *U, Route::Node *V) const;

    // Tests if the segments of U and V are adjacent in the same route.
    bool adjacent(Route::Node *U, Route::Node *V) const;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;

    std::string name() const override;

    static bool supports(ProblemData const &data);
};

template <size_t N, size_t M>
bool Swap<N, M>::hasDepot(Route::Node *node, size_t segLength) const
{
    auto const first = node->pos();
    auto const last = first + segLength - 1;
    auto const &route = *node->route();

    return first == 0                               // contains start depot
           || last >= route.size() - 1              // contains end depot
           || node->trip() != route[last]->trip();  // contains reload depot
}

template <size_t N, size_t M>
bool Swap<N, M>::overlap(Route::Node *U, Route::Node *V) const
{
    assert(U->route() == V->route());
    return U->pos() <= V->pos() + M - 1 && V->pos() <= U->pos() + N - 1;
}

template <size_t N, size_t M>
bool Swap<N, M>::adjacent(Route::Node *U, Route::Node *V) const
{
    assert(U->route() == V->route());
    return U->pos() + N == V->pos() || V->pos() + M == U->pos();
}

template <size_t N, size_t M>
bool Swap<N, M>::splitsShipment(Route::Node *node, size_t segLength) const
{
    auto const &route = *node->route();
    auto const last = node->pos() + segLength - 1;

    // Moving this segment certainly does not split a shipment if there is not
    // currently a shipment on the vehicle (at node), or if one is loaded, it
    // is delivered within this segment.
    return node->isDelivery()
           || route.numPickups(node->pos())
                  != route.numDeliveries(node->pos()) + node->isPickup()
           || route.numPickups(last) != route.numDeliveries(last);
}

template <size_t N, size_t M>
std::pair<Cost, bool> Swap<N, M>::evaluate(Route::Node *U,
                                           Route::Node *V,
                                           CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (!U->route() || !V->route() || hasDepot(U, N) || splitsShipment(U, N)
        || hasDepot(V, M) || splitsShipment(V, M))
        return std::make_pair(0, false);

    if (U->route() == V->route()
        && (U->trip() != V->trip() || overlap(U, V) || adjacent(U, V)))
        // We cannot easily evaluate across trips, and if U and V overlap or
        // are adjacent the move is not well-defined.
        return std::make_pair(0, false);

    if constexpr (N == M)  // symmetric, so only have to evaluate this once
        if (U->idx() >= V->idx())
            return std::make_pair(0, false);

    Cost deltaCost = 0;
    if (U->route() != V->route())
    {
        auto const *uRoute = U->route();
        auto const *vRoute = V->route();

        auto const uProposal
            = Route::Proposal(uRoute->before(U->pos() - 1),
                              vRoute->between(V->pos(), V->pos() + M - 1),
                              uRoute->after(U->pos() + N));

        auto const vProposal
            = Route::Proposal(vRoute->before(V->pos() - 1),
                              uRoute->between(U->pos(), U->pos() + N - 1),
                              vRoute->after(V->pos() + M));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else  // within same route
    {
        auto const *route = U->route();

        if (U->pos() < V->pos())
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(U->pos() - 1),
                                route->between(V->pos(), V->pos() + M - 1),
                                route->between(U->pos() + N, V->pos() - 1),
                                route->between(U->pos(), U->pos() + N - 1),
                                route->after(V->pos() + M)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(V->pos() - 1),
                                route->between(U->pos(), U->pos() + N - 1),
                                route->between(V->pos() + M, U->pos() - 1),
                                route->between(V->pos(), V->pos() + M - 1),
                                route->after(U->pos() + N)));
    }

    return std::make_pair(deltaCost, deltaCost < 0);
}

template <size_t N, size_t M>
void Swap<N, M>::apply(Route::Node *U, Route::Node *V) const
{
    stats_.numApplications++;

    auto &uRoute = *U->route();
    auto &vRoute = *V->route();
    auto *uToInsert = N == 1 ? U : uRoute[U->pos() + N - 1];
    auto *insertUAfter = vRoute[V->pos() + M - 1];

    // Insert these 'extra' nodes of U after the end of V...
    for (size_t count = 0; count != N - M; ++count)
    {
        auto *prev = p(uToInsert);
        uRoute.remove(uToInsert->pos());
        vRoute.insert(insertUAfter->pos() + 1, uToInsert);
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

template <size_t N, size_t M> std::string Swap<N, M>::name() const
{
    return "Swap" + std::to_string(N) + std::to_string(M);
}

template <size_t N, size_t M> bool Swap<N, M>::supports(ProblemData const &data)
{
    if (data.numClients() == 0 && data.numShipments() > 0)
        if constexpr (N & 1 || M & 1)  // cannot move uneven number of nodes
            return false;              // if the instance has only shipments

    return true;
}
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SWAP_H
