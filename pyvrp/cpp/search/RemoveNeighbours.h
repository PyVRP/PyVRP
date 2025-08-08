#ifndef PYVRP_SEARCH_REMOVE_NEIGHBOURS_H
#define PYVRP_SEARCH_REMOVE_NEIGHBOURS_H

#include "PerturbationOperator.h"
#include "ProblemData.h"

namespace pyvrp::search
{
/**
 * RemoveNeighbours(data: ProblemData)
 *
 * This operator removes the closest neighbours around a randomly selected
 * client. The removed clients are *not* reinserted back into the solution
 * - this is handled by the local search's ``search()`` method.
 */
class RemoveNeighbours : public PerturbationOperator
{
    ProblemData const &data_;

public:
    RemoveNeighbours(ProblemData const &data);

    void operator()(PerturbationContext const &context) override;
};

}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_REMOVE_NEIGHBOURS_H
