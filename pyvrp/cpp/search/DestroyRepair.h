#ifndef PYVRP_SEARCH_DESTROYREPAIR_H
#define PYVRP_SEARCH_DESTROYREPAIR_H

#include "CostEvaluator.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Route.h"
#include "Solution.h"
#include "SwapTails.h"

#include <vector>

namespace pyvrp::search
{
class DestroyRepair
{
    using Neighbours = std::vector<std::vector<size_t>>;

    SwapTails op;

    ProblemData const &data;
    RandomNumberGenerator &rng;

    // Neighborhood restrictions: list of nearby clients for each client (size
    // numLocations, but nothing is stored for the depots!)
    Neighbours neighbours_;

    std::vector<size_t> orderNodes;  // node order used by destroy/repair
    std::vector<Route::Node> nodes;
    std::vector<Route> routes;

    // Destroy the solution.
    void destroy(size_t numDestroy);
    void concentric(size_t numDestroy);
    void strings(size_t numDestroy);
    void swaproutes(size_t numDestroy);

    // Repair the solution.
    void repair(CostEvaluator const &costEvaluator);
    void greedyInsert(CostEvaluator const &costEvaluator);

public:
    /**
     * Destroys and repairs a solution with randomly selected operators.
     */
    Solution operator()(Solution const &solution,
                        CostEvaluator const &costEvaluator,
                        size_t numDestroy);

    DestroyRepair(ProblemData const &data,
                  RandomNumberGenerator &rng,
                  Neighbours neighbours);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_DESTROYREPAIR_H
