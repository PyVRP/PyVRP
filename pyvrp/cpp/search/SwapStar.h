#ifndef PYVRP_SWAPSTAR_H
#define PYVRP_SWAPSTAR_H

#include "LocalSearchOperator.h"
#include "Matrix.h"
#include "Measure.h"

#include <array>
#include <limits>
#include <vector>

namespace pyvrp::search
{
/**
 * SwapStar(data: ProblemData)
 *
 * Explores the SWAP* neighbourhood of [1]_. The SWAP* neighbourhood explores
 * free form re-insertions of clients :math:`U` and :math:`V` in the given
 * routes (so the clients are exchanged between routes, but they are not
 * necessarily inserted in the place of the other exchanged client).
 *
 * References
 * ----------
 * .. [1] Thibaut Vidal. 2022. Hybrid genetic search for the CVRP: Open-source
 *        implementation and SWAP* neighborhood. *Comput. Oper. Res*. 140.
 *        https://doi.org/10.1016/j.cor.2021.105643
 */
class SwapStar : public LocalSearchOperator<Route>
{
    struct ThreeBest  // stores three best SWAP* insertion points
    {
        bool shouldUpdate = true;
        std::array<Route::Node *, 3> locs = {nullptr, nullptr, nullptr};
        std::array<Cost, 3> costs = {std::numeric_limits<Cost>::max(),
                                     std::numeric_limits<Cost>::max(),
                                     std::numeric_limits<Cost>::max()};

        void maybeAdd(Cost costInsert, Route::Node *placeInsert);
    };

    struct BestMove  // tracks the best SWAP* move
    {
        Cost cost = 0;

        Route::Node *U = nullptr;
        Route::Node *UAfter = nullptr;  // insert U after this node in V's route

        Route::Node *V = nullptr;
        Route::Node *VAfter = nullptr;  // insert V after this node in U's route
    };

    Matrix<ThreeBest> cache;
    Matrix<Cost> removalCosts;
    std::vector<bool> updated;

    BestMove best;

    // Updates the removal costs of clients in the given route
    void updateRemovalCosts(Route *R, CostEvaluator const &costEvaluator);

    // Updates the cache storing the three best positions in the given route for
    // the passed-in node (client).
    void updateInsertionCost(Route *R,
                             Route::Node *U,
                             CostEvaluator const &costEvaluator);

    // Gets the delta cost and reinsert point for U in the route of V, assuming
    // V is removed.
    std::pair<Cost, Route::Node *> getBestInsertPoint(
        Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator);

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

    explicit SwapStar(ProblemData const &data)
        : LocalSearchOperator<Route>(data),
          cache(data.numVehicles(), data.numLocations()),
          removalCosts(data.numVehicles(), data.numLocations()),
          updated(data.numVehicles(), true)
    {
    }
};
}  // namespace pyvrp::search

#endif  // PYVRP_SWAPSTAR_H
