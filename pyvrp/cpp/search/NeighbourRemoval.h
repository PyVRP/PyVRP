#ifndef PYVRP_SEARCH_NEIGHBOUR_REMOVAL_H
#define PYVRP_SEARCH_NEIGHBOUR_REMOVAL_H

#include "PerturbationOperator.h"
#include "ProblemData.h"

#include <vector>

namespace pyvrp::search
{
/**
 * Neighbour removal perturbation operator. This operator removes the closest
 * neighbours around a randomly selected client. The removed clients are *not*
 * reinserted back into the solution - this is handled by the local search's
 * ``search()`` method.
 */
class NeighbourRemoval : public PerturbationOperator
{
    ProblemData const &data_;
    size_t const numPerturb_;

public:
    /**
     * Creates a neighbour removal operator.
     *
     * Parameters
     * ----------
     * data
     *     Problem data instance.
     * num_perturb
     *     Maximum number of clients to remove from the solution.
     */
    NeighbourRemoval(ProblemData const &data, size_t const numPerturb);

    void operator()(std::vector<search::Route::Node> &nodes,
                    std::vector<search::Route> &routes,
                    CostEvaluator const &costEvaluator,
                    std::vector<std::vector<size_t>> const &neighbours,
                    std::vector<size_t> const &orderNodes,
                    DynamicBitset &promising) override;
};

}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_NEIGHBOUR_REMOVAL_H
