#ifndef PYVRP_SEARCH_OPTIONAL_INSERT_H
#define PYVRP_SEARCH_OPTIONAL_INSERT_H

#include "PerturbationOperator.h"
#include "ProblemData.h"

namespace pyvrp::search
{
/**
 * Optional insert perturbation operator. This operator forcefully inserts
 * optional clients into the solution.
 */
class OptionalInsert : public PerturbationOperator
{
    ProblemData const &data_;

public:
    /**
     * Creates an optional insert operator.
     *
     * Parameters
     * ----------
     * data
     *     Problem data instance.
     */
    OptionalInsert(ProblemData const &data);

    void operator()(PerturbationContext const &context) override;
};

template <> bool supports<OptionalInsert>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_OPTIONAL_INSERT_H
