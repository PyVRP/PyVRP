#ifndef PYVRP_PERTURB_DESTROYREPAIR_H
#define PYVRP_PERTURB_DESTROYREPAIR_H

#include "CostEvaluator.h"
#include "DestroyOperator.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "RepairOperator.h"
#include "Solution.h"
#include "search/Route.h"

#include <vector>

namespace pyvrp::perturb
{
/**
 * DestroyRepair(data: ProblemData)
 *
 * Creates a DestroyRepair instance. This class perturbs a solution by using
 * destroy and repair operators. It randomly selects a destroy and repair
 * operator pair, and then applies them in sequence. The resulting solution is
 * not necessarily improved, but hopefully modified enough to escape local
 * optima.
 *
 * Parameters
 * ----------
 * data
 *     Problem data instance to be solved.
 */
class DestroyRepair
{
    ProblemData const &data;

    std::vector<search::Route::Node> nodes;
    std::vector<search::Route> routes;

    std::vector<DestroyOperator *> destroyOps;
    std::vector<RepairOperator *> repairOps;

    // Load an initial solution that we will attempt to improve.
    void loadSolution(Solution const &solution);

    // Export the LS solution back into a solution.
    Solution exportSolution() const;

public:
    /**
     * Destroys and repairs a solution with randomly selected operators.
     */
    Solution operator()(Solution const &solution,
                        CostEvaluator const &costEvaluator,
                        std::vector<std::vector<size_t>> const &neighbours,
                        RandomNumberGenerator &rng);

    /**
     * Adds a destroy operator to the list of available destroy operators.
     */
    void addDestroyOperator(DestroyOperator &op);

    /**
     * Adds a repair operator to the list of available repair operators.
     */
    void addRepairOperator(RepairOperator &op);

    DestroyRepair(ProblemData const &data);
};
}  // namespace pyvrp::perturb

#endif  // PYVRP_PERTURB_DESTROYREPAIR_H
