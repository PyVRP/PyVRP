#ifndef PYVRP_SEARCH_CHANGEVEHICLETYPE_H_
#define PYVRP_SEARCH_CHANGEVEHICLETYPE_H_

#include "PerturbationOperator.h"
#include "ProblemData.h"
#include "SwapTails.h"

namespace pyvrp::search
{
/**
 * Change vehicle type perturbation operator. This operator selects a random
 * non-empty route and changes its vehicle type.
 */
class ChangeVehicleType : public PerturbationOperator
{
    ProblemData const &data_;
    SwapTails op;

public:
    /**
     * Creates a change vehicle type perturbation operator.
     *
     * Parameters
     * ----------
     * data
     *     Problem data instance.
     */
    ChangeVehicleType(ProblemData const &data);

    void operator()(PerturbationContext const &context) override;
};

template <> bool supports<ChangeVehicleType>(ProblemData const &data);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_CHANGEVEHICLETYPE_H_
