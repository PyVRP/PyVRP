#ifndef PYVRP_SEARCH_RELOCATE_H
#define PYVRP_SEARCH_RELOCATE_H

#include "LocalSearchOperator.h"
#include "Route.h"

#include <cassert>

namespace pyvrp::search
{
/**
 * Relocate(data: ProblemData)
 *
 * TODO
 */
template <size_t N> class Relocate : public BinaryOperator
{
    using BinaryOperator::BinaryOperator;

    static_assert(N > 0, "N == 0 does not make sense");

    // Tests if the segment starting at node contains a depot.
    bool hasDepot(Route::Node *node) const;

    // Tests if the segment starting at node splits a shipment if relocated.
    bool splitsShipment(Route::Node *node) const;

    // Tests if the segments of U and V overlap in the same route.
    bool overlap(Route::Node *U, Route::Node *V) const;

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;

    std::string name() const override;

    static bool supports(ProblemData const &data);
};

template <size_t N> bool Relocate<N>::hasDepot(Route::Node *node) const
{
    auto const first = node->pos();
    auto const last = first + N - 1;
    auto const &route = *node->route();

    return first == 0                               // contains start depot
           || last >= route.size() - 1              // contains end depot
           || node->trip() != route[last]->trip();  // contains reload depot
}

template <size_t N>
bool Relocate<N>::overlap(Route::Node *U, Route::Node *V) const
{
    assert(U->route() == V->route());
    return U->pos() <= V->pos() && V->pos() <= U->pos() + N - 1;
}

template <size_t N> bool Relocate<N>::splitsShipment(Route::Node *node) const
{
    auto const &route = *node->route();
    auto const last = node->pos() + N - 1;

    // Moving this segment certainly does not split a shipment if there is not
    // currently a shipment on the vehicle (at node), or if one is loaded, it
    // is delivered within this segment.
    return node->isDelivery()
           || route.numPickups(node->pos())
                  != route.numDeliveries(node->pos()) + node->isPickup()
           || route.numPickups(last) != route.numDeliveries(last);
}

template <size_t N>
std::pair<Cost, bool> Relocate<N>::evaluate(Route::Node *U,
                                            Route::Node *V,
                                            CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (!U->route() || !V->route() || hasDepot(U) || splitsShipment(U))
        return std::make_pair(0, false);

    if (U->route() == V->route()
        && (U->trip() != V->trip() || overlap(U, V) || U == n(V)))
        // We cannot easily evaluate across trips, and if U and V overlap the
        // move is not well-defined. If they are adjacent the move is a no-op.
        return std::make_pair(0, false);

    Cost deltaCost = 0;
    if (U->route() != V->route())
    {
        auto const *uRoute = U->route();
        auto const *vRoute = V->route();

        auto const vProposal
            = Route::Proposal(vRoute->before(V->pos()),
                              uRoute->between(U->pos(), U->pos() + N - 1),
                              vRoute->after(V->pos() + 1));

        // Then U's route is empty after this move, so we can subtract the
        // current route's cost and only evaluate V's proposal.
        if (uRoute->numClients() + 2 * uRoute->numShipments() == N)
        {
            deltaCost -= costEvaluator.penalisedCost(*uRoute);
            costEvaluator.deltaCost(deltaCost, vProposal);
        }
        else
        {
            auto const uProposal = Route::Proposal(uRoute->before(U->pos() - 1),
                                                   uRoute->after(U->pos() + N));

            costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
        }
    }
    else  // within same route
    {
        auto *route = U->route();

        if (U->pos() < V->pos())
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(U->pos() - 1),
                                route->between(U->pos() + N, V->pos()),
                                route->between(U->pos(), U->pos() + N - 1),
                                route->after(V->pos() + 1)));
        else
            costEvaluator.deltaCost(
                deltaCost,
                Route::Proposal(route->before(V->pos()),
                                route->between(U->pos(), U->pos() + N - 1),
                                route->between(V->pos() + 1, U->pos() - 1),
                                route->after(U->pos() + N)));
    }

    return std::make_pair(deltaCost, deltaCost < 0);
}

template <size_t N>
void Relocate<N>::apply(Route::Node *U, Route::Node *V) const
{
    stats_.numApplications++;

    auto &uRoute = *U->route();
    auto &vRoute = *V->route();
    auto *uToInsert = N == 1 ? U : uRoute[U->pos() + N - 1];

    for (size_t count = 0; count != N; ++count)  // insert segment after V
    {
        auto *prev = p(uToInsert);
        uRoute.remove(uToInsert->pos());
        vRoute.insert(V->pos() + 1, uToInsert);
        uToInsert = prev;
    }
}

template <size_t N> std::string Relocate<N>::name() const
{
    return "Relocate" + std::to_string(N);
}

template <size_t N> bool Relocate<N>::supports(ProblemData const &data)
{
    if (data.numClients() == 0 && data.numShipments() > 0)
        if constexpr (N & 1)  // cannot move uneven number of nodes
            return false;     // if the instance has only shipments

    return true;
}
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_RELOCATE_H
