#ifndef PYVRP_SWAPSTAR_H
#define PYVRP_SWAPSTAR_H

#include "LocalSearchOperator.h"
#include "Matrix.h"
#include "Measure.h"

#include <array>
#include <limits>
#include <vector>

/**
 * Explores the SWAP* neighbourhood of [1]. The SWAP* neighbourhood explores
 * free form re-insertions of nodes U and V in the given routes (so the nodes
 * are exchanged between routes, but they are not necessarily inserted in
 * the same place as the other exchanged node). Our implementation of the
 * neighbourhood follows Algorithm 2 of [1] fairly faithfully.
 * <br />
 * Thibaut Vidal. 2022. Hybrid genetic search for the CVRP: Open-source
 * implementation and SWAP* neighborhood. Comput. Oper. Res. 140.
 * https://doi.org/10.1016/j.cor.2021.105643
 */
class SwapStar : public LocalSearchOperator<Route>
{
    struct ThreeBest  // stores three best SWAP* insertion points
    {
        bool shouldUpdate = true;
        std::array<Cost, 3> costs = {std::numeric_limits<Cost>::max(),
                                     std::numeric_limits<Cost>::max(),
                                     std::numeric_limits<Cost>::max()};
        std::array<Node *, 3> locs = {nullptr, nullptr, nullptr};

        void maybeAdd(Cost costInsert, Node *placeInsert)
        {
            if (costInsert >= costs[2])
                return;

            if (costInsert >= costs[1])
            {
                costs[2] = costInsert;
                locs[2] = placeInsert;
            }
            else if (costInsert >= costs[0])
            {
                costs[2] = costs[1];
                locs[2] = locs[1];
                costs[1] = costInsert;
                locs[1] = placeInsert;
            }
            else
            {
                costs[2] = costs[1];
                locs[2] = locs[1];
                costs[1] = costs[0];
                locs[1] = locs[0];
                costs[0] = costInsert;
                locs[0] = placeInsert;
            }
        }
    };

    struct BestMove  // tracks the best SWAP* move
    {
        Cost cost = 0;

        Node *U = nullptr;
        Node *UAfter = nullptr;

        Node *V = nullptr;
        Node *VAfter = nullptr;
    };

    // Updates the removal costs of clients in the given route
    void updateRemovalCosts(Route *R1, CostEvaluator const &costEvaluator);

    // Updates the cache storing the three best positions in the given route for
    // the passed-in node (client).
    void
    updateInsertionCost(Route *R, Node *U, CostEvaluator const &costEvaluator);

    // Gets the delta cost and reinsert point for U in the route of V, assuming
    // V is removed.
    inline std::pair<Cost, Node *>
    getBestInsertPoint(Node *U, Node *V, CostEvaluator const &costEvaluator);

    Matrix<ThreeBest> cache;
    Matrix<Cost> removalCosts;
    std::vector<bool> updated;

    BestMove best;

public:
    void init(Solution const &solution) override;

    Cost
    evaluate(Route *U, Route *V, CostEvaluator const &costEvaluator) override;

    void apply(Route *U, Route *V) const override;

    void update(Route *U) override;

    explicit SwapStar(ProblemData const &data)
        : LocalSearchOperator<Route>(data),
          cache(data.numVehicles(), data.numClients() + 1),
          removalCosts(data.numVehicles(), data.numClients() + 1),
          updated(data.numVehicles(), true)
    {
    }
};

#endif  // PYVRP_SWAPSTAR_H
