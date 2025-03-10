#ifndef PYVRP_SEARCH_DESTROYREPAIR_H
#define PYVRP_SEARCH_DESTROYREPAIR_H

#include "CostEvaluator.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Route.h"
#include "Solution.h"

#include <vector>

namespace pyvrp::search
{
class DestroyRepair
{
    using Neighbours = std::vector<std::vector<size_t>>;

    ProblemData const &data;
    RandomNumberGenerator &rng;

    // Neighborhood restrictions: list of nearby clients for each client (size
    // numLocations, but nothing is stored for the depots!)
    Neighbours neighbours_;

    std::vector<size_t> orderNodes;  // node order used by destroy/repair
    std::vector<Route::Node> nodes;
    std::vector<Route> routes;

    // Load an initial solution that we will attempt to improve.
    void loadSolution(Solution const &solution);

    // Export the LS solution back into a solution.
    Solution exportSolution() const;

    // Destroy the solution.
    void destroy(size_t numDestroy);
    void concentric(size_t numDestroy);
    void strings(size_t numDestroy);

    // Repair the solution.
    void repair(CostEvaluator const &costEvaluator);
    void greedyInsert(CostEvaluator const &costEvaluator);

    // Updates solution state after an improving local search move.
    void update(Route *U, Route *V);

public:
    /**
     * Set neighbourhood structure to use by the local search. For each client,
     * the neighbourhood structure is a vector of nearby clients. Depots have
     * no nearby client.
     */
    void setNeighbours(Neighbours neighbours);

    /**
     * @return The neighbourhood structure currently in use.
     */
    Neighbours const &neighbours() const;

    /**
     * Iteratively calls ``search()`` and ``intensify()`` until no further
     * improvements are made.
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
