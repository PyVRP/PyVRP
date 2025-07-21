#ifndef PYVRP_SEARCH_STRING_REMOVAL_H
#define PYVRP_SEARCH_STRING_REMOVAL_H

#include "PerturbationOperator.h"
#include "ProblemData.h"

#include <vector>

namespace pyvrp::search
{
/**
 * String removal perturbation operator. This operator removes a string of
 * consecutive clients from a randomly selected route. The removed clients are
 * *not* reinserted back into the solution - this is handled by the local
 * search's ``search()`` method.
 */
class StringRemoval : public PerturbationOperator
{
    ProblemData const &data_;

public:
    /**
     * Creates a string removal operator.
     *
     * Parameters
     * ----------
     * data
     *     Problem data instance.
     */
    StringRemoval(ProblemData const &data);

    void operator()(PerturbationContext const &context) override;
};

}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_STRING_REMOVAL_H
