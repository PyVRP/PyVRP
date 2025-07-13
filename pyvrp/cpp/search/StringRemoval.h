#ifndef PYVRP_SEARCH_STRING_REMOVAL_H
#define PYVRP_SEARCH_STRING_REMOVAL_H

#include "PerturbationOperator.h"
#include "ProblemData.h"

namespace pyvrp::search
{
/**
 * String removal perturbation operator. This operator removes consecutive
 * clients (strings) from routes starting from the closest neighbours around
 * a randomly selected client. The removed clients are *not* reinserted back
 * into the solution - this is handled by the local search's ``search()``
 * method.
 */
class StringRemoval : public PerturbationOperator
{
    ProblemData const &data_;
    size_t const numPerturb_;

public:
    /**
     * Creates a string removal operator.
     *
     * Parameters
     * ----------
     * data
     *     Problem data instance.
     * num_perturb
     *     Maximum number of clients to remove from the solution.
     */
    StringRemoval(ProblemData const &data, size_t const numPerturb);

    void operator()(PerturbationContext const &context) override;
};

}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_STRING_REMOVAL_H
