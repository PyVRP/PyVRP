#ifndef PYVRP_SEARCH_PERTURBATIONMANAGER_H
#define PYVRP_SEARCH_PERTURBATIONMANAGER_H

#include "RandomNumberGenerator.h"

#include <iosfwd>

namespace pyvrp::search
{
/**
 * PerturbationManager(
 *     min_perturbations: int = 1,
 *     max_perturbations: int = 25,
 * )
 *
 * TODO
 */
class PerturbationManager
{
    size_t minPerturbations_;
    size_t maxPerturbations_;
    size_t numPerturbations_;

public:
    PerturbationManager(size_t minPerturbations = 1,
                        size_t maxPerturbations = 25);

    size_t numPerturbations() const;

    void shuffle(RandomNumberGenerator &rng);
};
};  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_LOCALSEARCH_H
