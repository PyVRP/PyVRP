#ifndef PYVRP_PERTURB_GREEDY_REPAIR_H
#define PYVRP_PERTURB_GREEDY_REPAIR_H

#include "DestroyRepairOperator.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"

#include <vector>

namespace pyvrp::perturb
{
/**
 * Greedy repair operator. This operator inserts all unplanned required
 * clients, and, with a given probability, optional clients as well. For each
 * client, it evaluates all insertion places after its neighbours as well as in
 * one randomly selected empty route. The client is then inserted at the best
 * position found.
 */
class GreedyRepair : public DestroyRepairOperator
{
    ProblemData const &data_;
    size_t skipOptionalProbability;

public:
    /**
     * Creates a greedy repair operator.
     *
     * Parameters
     * ----------
     * data
     *     Problem data instance.
     * skipOptionalProbability
     *     Probability of skipping optional clients during repair.
     *     Higher values mean fewer optional clients are included.
     */
    GreedyRepair(ProblemData const &data, size_t skipOptionalProbability = 100);

    void operator()(std::vector<search::Route::Node> &nodes,
                    std::vector<search::Route> &routes,
                    CostEvaluator const &costEvaluator,
                    std::vector<std::vector<size_t>> const &neighbours,
                    RandomNumberGenerator &rng) override;
};

}  // namespace pyvrp::perturb

#endif  // PYVRP_PERTURB_GREEDY_REPAIR_H
