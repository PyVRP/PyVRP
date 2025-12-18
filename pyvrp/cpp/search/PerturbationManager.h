#ifndef PYVRP_SEARCH_PERTURBATIONMANAGER_H
#define PYVRP_SEARCH_PERTURBATIONMANAGER_H

#include "CostEvaluator.h"
#include "LocalSearch.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Route.h"
#include "SearchSpace.h"

#include <iosfwd>

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
 * Manages the number of perturbations to apply during the search.
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

    /**
     * Attempts to determine a good insertion point for U. May fail if no such
     * point can be found, in which case a null pointer is returned.
     */
    Route::Node *insertAfter(Route::Node *U,
                             LocalSearch::Solution &solution,
                             SearchSpace &searchSpace,
                             LocalSearch::SearchOrder &searchOrder,
                             ProblemData const &data,
                             CostEvaluator const &costEvaluator);

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
     * TODO
     */
    void perturb(LocalSearch::Solution &solution,
                 SearchSpace &searchSpace,
                 LocalSearch::SearchOrder &searchOrder,
                 ProblemData const &data,
                 CostEvaluator const &costEvaluator);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_PERTURBATIONMANAGER_H
