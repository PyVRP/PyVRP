#ifndef PYVRP_SEARCH_EXCHANGESTAR_H
#define PYVRP_SEARCH_EXCHANGESTAR_H

#include "Exchange.h"
#include "LocalSearchOperator.h"

namespace pyvrp::search
{
/**
 * ExchangeStar(data: ProblemData)
 *
 * Performs the best (N, M)-exchange move between routes U and V.
 */
template <size_t N, size_t M>
class ExchangeStar : public LocalSearchOperator<Route>
{
    struct Move
    {
        Cost deltaCost = 0;
        Route::Node *from = nullptr;
        Route::Node *to = nullptr;
    };

    Exchange<N, M> exchange;
    Move move;

public:
    Cost
    evaluate(Route *U, Route *V, CostEvaluator const &costEvaluator) override;

    void apply(Route *U, Route *V) const override;

    ExchangeStar(ProblemData const &data)
        : LocalSearchOperator<Route>(data), exchange(data)
    {
    }
};

template <size_t N, size_t M>
Cost ExchangeStar<N, M>::evaluate(Route *U,
                                  Route *V,
                                  CostEvaluator const &costEvaluator)
{
    move = {};
    Cost deltaCost = 0;

    for (auto *nodeU : *U)
    {
        if constexpr (M == 0)  // Test U after V's start depot only when
        {                      // relocating.
            deltaCost = exchange.evaluate(nodeU, &V->startDepot, costEvaluator);

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeU, &V->startDepot};
        }

        for (auto *nodeV : *V)
        {
            // Test (U, V).
            deltaCost = exchange.evaluate(nodeU, nodeV, costEvaluator);

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeU, nodeV};

            // Test (V, U). This is equivalent to (U, V) in case of pure swap
            // - shortcutting that is already handled by the implementation of
            // the node operator.
            deltaCost = exchange.evaluate(nodeV, nodeU, costEvaluator);

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeV, nodeU};
        }
    }

    return move.deltaCost;
}

template <size_t N, size_t M>
void ExchangeStar<N, M>::apply([[maybe_unused]] Route *U,
                               [[maybe_unused]] Route *V) const
{
    exchange.apply(move.from, move.to);
}
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_EXCHANGESTAR_H
