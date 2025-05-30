#ifndef PYVRP_PERTURB_NEIGHBOUR_REMOVAL_H
#define PYVRP_PERTURB_NEIGHBOUR_REMOVAL_H

#include "DestroyRepairOperator.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"

#include <vector>

namespace pyvrp::perturb
{
/**
 * Neighbour removal destroy operator. This operator removes the closest
 * neighbours around a randomly selected client.
 */
class NeighbourRemoval : public DestroyRepairOperator
{
    ProblemData const &data;
    size_t const numRemovals;

public:
    /**
     * Creates a neighbour removal operator.
     *
     * Parameters
     * ----------
     * data
     *     Problem data instance.
     * rng
     *     Random number generator.
     * num_removals
     *     Number of clients to remove from the solution.
     */
    NeighbourRemoval(ProblemData const &data, size_t const numRemovals);

    void operator()(std::vector<search::Route::Node> &nodes,
                    std::vector<search::Route> &routes,
                    CostEvaluator const &costEvaluator,
                    std::vector<std::vector<size_t>> const &neighbours,
                    RandomNumberGenerator &rng) override;
};

}  // namespace pyvrp::perturb

#endif  // PYVRP_PERTURB_NEIGHBOUR_REMOVAL_H
