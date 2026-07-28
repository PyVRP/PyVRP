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

    // related_[a] holds the nearest activities to activity a, by symmetric
    // distance. Activity a in [0, numShipments) is a pickup, and in
    // [numShipments, 2 * numShipments) the delivery of a - numShipments.
    std::vector<std::vector<size_t>> related_;

    mutable RandomNumberGenerator rng_;

public:
    explicit ShipmentRuinAndRecreate(ProblemData const &data);

    // Reseeds from the given generator, so runs stay reproducible.
    void reseed(RandomNumberGenerator &rng);

    // Returns true if the solution was perturbed. When false, nothing was
    // mutated and the caller should fall back to the usual perturbation.
    bool apply(Solution &solution,
               SearchSpace &searchSpace,
               CostEvaluator const &costEvaluator) const;

    // Reinserts every currently unassigned shipment, scanning all pickup and
    // delivery position pairs in a few related routes.
    void recreate(Solution &solution,
                  SearchSpace &searchSpace,
                  CostEvaluator const &costEvaluator) const;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SHIPMENTRUINANDRECREATE_H
