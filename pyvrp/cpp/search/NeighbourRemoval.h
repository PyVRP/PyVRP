#ifndef PYVRP_SEARCH_NEIGHBOUR_REMOVAL_H
#define PYVRP_SEARCH_NEIGHBOUR_REMOVAL_H

#include "DestroyOperator.h"
#include "ProblemData.h"

#include <vector>

namespace pyvrp::search
{
/**
 * Neighbour removal destroy operator. This operator removes the closest
 * neighbours around a randomly selected client.
 */
class NeighbourRemoval : public DestroyOperator
{
    size_t const numDestroy;

public:
    /**
     * Creates a neighbour removal operator.
     *
     * Parameters
     * ----------
     * num_destroy
     *     Maximum number of clients to remove from the solution.
     */
    NeighbourRemoval(size_t const numDestroy);

    void operator()(std::vector<search::Route::Node> &nodes,
                    std::vector<search::Route> &routes,
                    CostEvaluator const &costEvaluator,
                    std::vector<std::vector<size_t>> const &neighbours,
                    std::vector<size_t> const &orderNodes) override;
};

}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_NEIGHBOUR_REMOVAL_H
