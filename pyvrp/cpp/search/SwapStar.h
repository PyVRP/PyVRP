#ifndef PYVRP_SEARCH_SWAPSTAR_H
#define PYVRP_SEARCH_SWAPSTAR_H

#include "LocalSearchOperator.h"
#include "Matrix.h"

#include <array>
#include <utility>

namespace pyvrp::search
{
/**
 * SwapStar(data: ProblemData, overlap_tolerance: float = 0.05)
 *
 * Explores the SWAP* neighbourhood of [1]_. The SWAP* neighbourhood consists
 * of free form re-insertions of clients :math:`U` and :math:`V` in the given
 * routes (so the clients are swapped, but they are not necessarily inserted
 * in the place of the other swapped client).
 *
 * References
 * ----------
 * .. [1] Thibaut Vidal. 2022. Hybrid genetic search for the CVRP: Open-source
 *        implementation and SWAP* neighborhood. *Comput. Oper. Res*. 140.
 *        https://doi.org/10.1016/j.cor.2021.105643
 */
class SwapStar : public LocalSearchOperator<Route>
{
    using InsertPoint = std::pair<Cost, Route::Node *>;
    using ThreeBest = std::array<InsertPoint, 3>;

    struct BestMove  // tracks the best SWAP* move
    {
        Cost cost = 0;

        Route::Node *U = nullptr;
        Route::Node *UAfter = nullptr;  // insert U after this node in V's route

        Route::Node *V = nullptr;
        Route::Node *VAfter = nullptr;  // insert V after this node in U's route
    };

    // To limit computational efforts, by default not all route pairs are
    // considered: only those route pairs that share some overlap when
    // considering their center's angle to the center of all clients. This value
    // controls the amount of overlap needed before two routes are evaluated.
    double const overlapTolerance;

    // Tracks the three best insert locations, for each route and client.
    Matrix<ThreeBest> insertCache;

    // Tracks whether the insert locations and removal costs are still up to
    // date. In particular, isCached(R, 0) tracks route-wise removal cost
    // validity, while isCached(R, U) with U > 0 tracks (route, client) insert
    // location validity.
    Matrix<bool> isCached;

    // Tracks the removal costs of removing a client from its route.
    Matrix<Cost> removalCosts;

    BestMove best;

    // Updates the removal costs of clients in the given route
    void updateRemovalCosts(Route *R, CostEvaluator const &costEvaluator);

    // Updates the cache storing the three best positions in the given route for
    // the passed-in node (client).
    void updateInsertPoints(Route *R,
                            Route::Node *U,
                            CostEvaluator const &costEvaluator);

    Cost deltaLoadCost(Route::Node *U,
                       Route::Node *V,
                       CostEvaluator const &costEvaluator) const;

    InsertPoint bestInsertPoint(Route::Node *U,
                                Route::Node *V,
                                CostEvaluator const &costEvaluator);

    // Evaluates the delta cost for ``V``'s route of inserting ``U`` after
    // ``V``, while removing ``remove`` from ``V``'s route.
    Cost evaluateMove(Route::Node const *U,
                      Route::Node const *V,
                      Route::Node const *remove,
                      CostEvaluator const &costEvaluator) const;

public:
    void init(Solution const &solution) override;

    Cost
    evaluate(Route *U, Route *V, CostEvaluator const &costEvaluator) override;

    void apply(Route *U, Route *V) const override;

    void update(Route *U) override;

    explicit SwapStar(ProblemData const &data, double overlapTolerance = 0.05);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SWAPSTAR_H
