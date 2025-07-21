#ifndef PYVRP_SEARCH_OPTIONAL_INSERT_H
#define PYVRP_SEARCH_OPTIONAL_INSERT_H

#include "PerturbationOperator.h"
#include "ProblemData.h"

namespace pyvrp::search
{
/**
 * Optional insert perturbation operator. This operator inserts optional clients
 * into the solution, if they are not already present.
 */
class OptionalInsert : public PerturbationOperator
{
    ProblemData const &data_;
    size_t const numPerturb_;

public:
    /**
     * Creates an optional insert operator.
     *
     * Parameters
     * ----------
     * data
     *     Problem data instance.
     * num_perturb
     *     Maximum number of clients to remove from the solution.
     */
    OptionalInsert(ProblemData const &data, size_t const numPerturb);

    void operator()(PerturbationContext const &context) override;
};

}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_OPTIONAL_INSERT_H
