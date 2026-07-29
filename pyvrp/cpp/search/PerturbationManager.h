#ifndef PYVRP_SEARCH_PERTURBATIONMANAGER_H
#define PYVRP_SEARCH_PERTURBATIONMANAGER_H

#include "CostEvaluator.h"
#include "RandomNumberGenerator.h"
#include "SearchSpace.h"
#include "Solution.h"

#include <iosfwd>
#include <vector>

namespace pyvrp::search
{
/**
 * PerturbationParams(min_perturbations: int = 1, max_perturbations: int = 25)
 *
 * Perturbation parameters.
 *
 * Parameters
 * ----------
 * min_perturbations
 *     Minimum number of perturbations to apply. Must not be negative.
 * max_perturbations
 *     Maximum number of perturbations to apply.
 */
struct PerturbationParams
{
    size_t const minPerturbations;
    size_t const maxPerturbations;

    PerturbationParams(size_t minPerturbations = 1,
                       size_t maxPerturbations = 25);

    bool operator==(PerturbationParams const &other) const = default;
};

/**
 * PerturbationManager(params: PerturbationParams)
 *
 * Handles perturbation during the search. In each iteration, it applies
 * :meth:`~num_perturbations` perturbations that strengthen (resp., weaken)
 * randomly selected neighbourhoods by inserting (removing) clients and
 * shipments.
 * Non-empty solutions use a ruin-and-recreate perturbation that exhaustively
 * recreates selected requests in the ruined routes.
 *
 * Parameters
 * ----------
 * params
 *     Perturbation parameters for this manager.
 */
class PerturbationManager
{
    struct RuinResult
    {
        std::vector<Route *> routes;
        std::vector<Activity> activities;
    };

    PerturbationParams const params_;  // owned by us
    size_t numPerturbations_;
    size_t numRoutes_;

    RuinResult ruin(Solution &solution, SearchSpace &searchSpace) const;

    void recreate(Solution &solution,
                  SearchSpace &searchSpace,
                  RuinResult const &result,
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
     * promising in the search space. Non-empty solutions use a
     * ruin-and-recreate perturbation that exhaustively recreates selected
     * requests in the ruined routes. Optional requests are only recreated when
     * their insertion improves the solution.
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
