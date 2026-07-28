#ifndef PYVRP_SEARCH_SHIPMENTRUINANDRECREATE_H
#define PYVRP_SEARCH_SHIPMENTRUINANDRECREATE_H

#include "CostEvaluator.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "SearchSpace.h"
#include "Solution.h"

#include <vector>

namespace pyvrp::search
{
/**
 * SISR-style ruin-and-recreate perturbation targeting cross-route shipment
 * membership. Experimental: constants are hardcoded, see the .cpp file.
 */
class ShipmentRuinAndRecreate
{
    ProblemData const &data;

    mutable RandomNumberGenerator rng_;

    bool ruin(Solution &solution,
              SearchSpace &searchSpace,
              std::vector<Route *> &routes) const;

public:
    explicit ShipmentRuinAndRecreate(ProblemData const &data);

    void apply(Solution &solution,
               SearchSpace &searchSpace,
               CostEvaluator const &costEvaluator) const;

    // Reinserts every currently unassigned shipment into the perturbed routes.
    void recreate(Solution &solution,
                  SearchSpace &searchSpace,
                  std::vector<Route *> const &routes,
                  CostEvaluator const &costEvaluator) const;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SHIPMENTRUINANDRECREATE_H
