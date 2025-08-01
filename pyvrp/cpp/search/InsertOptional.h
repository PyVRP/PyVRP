#ifndef PYVRP_SEARCH_INSERT_OPTIONAL_H
#define PYVRP_SEARCH_INSERT_OPTIONAL_H

#include "PerturbationOperator.h"
#include "ProblemData.h"

namespace pyvrp::search
{
/**
 * InsertOptional(data: ProblemData)
 *
 * This operator forcefully inserts optional clients into the solution.
 */
class InsertOptional : public PerturbationOperator
{
    ProblemData const &data_;

public:
    InsertOptional(ProblemData const &data);

    void operator()(PerturbationContext const &context) override;
};

template <> bool supports<InsertOptional>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_INSERT_OPTIONAL_H
