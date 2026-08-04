#ifndef PYVRP_SEARCH_PERTURBATIONMANAGER_H
#define PYVRP_SEARCH_PERTURBATIONMANAGER_H

#include "CostEvaluator.h"
#include "RandomNumberGenerator.h"
#include "SearchSpace.h"
#include "Solution.h"

#include <iosfwd>

namespace pyvrp::search
{
/**
 * PerturbationParams(
 *     min_perturbations: int = 1,
 *     max_perturbations: int = 25,
 *     min_routes: int = 1,
 *     max_routes: int = 3,
 * )
 *
 * Perturbation parameters.
 *
 * Parameters
 * ----------
 * min_perturbations
 *     Minimum number of perturbations to apply. Must not be negative.
 * max_perturbations
 *     Maximum number of perturbations to apply.
 * min_routes
 *     Minimum number of routes to perturb.
 * max_routes
 *     Maximum number of routes to perturb.
 *
 * Raises
 * ------
 * ValueError
 *     When ``min_perturbations`` exceeds ``max_perturbations``,
 *     or ``min_routes`` exceeds ``max_routes``.
 */
struct PerturbationParams
{
    size_t const minPerturbations;
    size_t const maxPerturbations;
    size_t const minRoutes;
    size_t const maxRoutes;

    PerturbationParams(size_t minPerturbations = 1,
                       size_t maxPerturbations = 25,
                       size_t minRoutes = 1,
                       size_t maxRoutes = 3);

    bool operator==(PerturbationParams const &other) const = default;
};

/**
 * PerturbationManager(params: PerturbationParams)
 *
 * Handles perturbation during the search. Neighbour perturbation inserts or
 * removes related clients and shipments. Route perturbation selects one or
 * more related seeds. For each planned seed, it removes part of the seed's
 * route. For each unplanned seed, it inserts the seed and neighbouring
 * activities.
 *
 * Parameters
 * ----------
 * params
 *     Perturbation parameters for this manager.
 */
class PerturbationManager
{
    PerturbationParams const params_;  // owned by us
    size_t numPerturbations_;
    size_t numRoutes_;
    bool useRoutePerturb_ = false;

    void neighbourPerturb(Solution &solution,
                          SearchSpace &searchSpace,
                          CostEvaluator const &costEvaluator) const;

    void routePerturb(Solution &solution,
                      SearchSpace &searchSpace,
                      CostEvaluator const &costEvaluator) const;

public:
    PerturbationManager(PerturbationParams params = PerturbationParams());

    /**
     * Number of perturbations to apply.
     */
    size_t numPerturbations() const;

    /**
     * Draws and sets a new random number of perturbations to apply.
     */
    void shuffle(RandomNumberGenerator &rng);

    /**
     * Perturbs the given solution using the neighbourhood and ordering of the
     * given search space. Any perturbed clients or shipments are marked as
     * promising in the search space.
     *
     * Parameters
     * ----------
     * solution
     *     Solution to perturb. Perturbation happens in place.
     * search_space
     *     The search space to use for perturbation.
     * cost_evaluator
     *     Evaluator to use for insertions.
     */
    void perturb(Solution &solution,
                 SearchSpace &searchSpace,
                 CostEvaluator const &costEvaluator) const;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_PERTURBATIONMANAGER_H
