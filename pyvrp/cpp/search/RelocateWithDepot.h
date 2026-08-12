#ifndef PYVRP_SEARCH_RELOCATEWITHDEPOT_H
#define PYVRP_SEARCH_RELOCATEWITHDEPOT_H

#include "DynamicBitset.h"
#include "LocalSearchOperator.h"

#include <vector>

namespace pyvrp::search
{
/**
 * RelocateWithDepot(data: ProblemData)
 *
 * Tests if inserting a reload depot while relocating :math:`U` after :math:`V`
 * results in an improving move. Concretely, this operator implements the second
 * and third insertion scheme of Francois et al. [1]_.
 *
 * References
 * ----------
 * .. [1] Francois, V., Y. Arda, and Y. Crama (2019). Adaptive Large
 *        Neighborhood Search for Multitrip Vehicle Routing with Time Windows.
 *        *Transportation Science*, 53(6): 1706 - 1730.
 *        https://doi.org/10.1287/trsc.2019.0909.
 */
class RelocateWithDepot : public BinaryOperator
{
    enum class MoveType
    {
        DEPOT_U,  // V -> depot -> U
        U_DEPOT,  // V -> U -> depot
    };

    struct Move
    {
        Cost cost = std::numeric_limits<Cost>::max();
        MoveType type = MoveType::DEPOT_U;
        size_t depot = 0;
    };

    Move move_;

    DynamicBitset isCached_;
    std::vector<Cost> removeCost_;

    // Evaluates relocation moves when U and V are in the same route.
    void evalSameRoute(Route::Node *U,
                       Route::Node *V,
                       CostEvaluator const &costEvaluator);

    // Evaluates relocation moves when U and V are in different routes.
    void evalDifferentRoutes(Route::Node *U,
                             Route::Node *V,
                             CostEvaluator const &costEvaluator);

public:
    std::pair<Cost, bool> evaluate(Route::Node *U,
                                   Route::Node *V,
                                   CostEvaluator const &costEvaluator) override;

    void apply(Route::Node *U, Route::Node *V) const override;

    void init(Solution &solution) override;

    std::string name() const override;

    static bool supports(ProblemData const &data);

    void update(Route const *route) override;

    RelocateWithDepot(ProblemData const &data);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_RELOCATEWITHDEPOT_H
