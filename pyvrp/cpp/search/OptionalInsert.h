#ifndef PYVRP_SEARCH_OPTIONAL_INSERT_H
#define PYVRP_SEARCH_OPTIONAL_INSERT_H

#include "PerturbationOperator.h"
#include "ProblemData.h"

#include <vector>

namespace pyvrp::search
{
/**
 * Optional insert perturbation operator. This operator removes a random
 * subset of clients from the solution and then attempts to reinsert them
 * using a probabilistic acceptance criterion. Clients that are not
 * reinserted are left unassigned in the solution.
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
