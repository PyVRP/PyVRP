#ifndef PYVRP_SEARCH_NEIGHBOUR_REMOVAL_H
#define PYVRP_SEARCH_NEIGHBOUR_REMOVAL_H

#include "PerturbationOperator.h"
#include "ProblemData.h"

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

public:
    /**
     * Creates a neighbour removal operator.
     *
     * Parameters
     * ----------
     * data
     *     Problem data instance.
     */
    NeighbourRemoval(ProblemData const &data);

    void operator()(PerturbationContext const &context) override;
};

}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_NEIGHBOUR_REMOVAL_H
