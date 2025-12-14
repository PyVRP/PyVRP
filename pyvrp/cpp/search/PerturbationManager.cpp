#include "PerturbationManager.h"

#include <stdexcept>

using pyvrp::search::PerturbationManager;

PerturbationManager::PerturbationManager(size_t minPerturbations,
                                         size_t maxPerturbations)
    : minPerturbations_(minPerturbations),
      maxPerturbations_(maxPerturbations),
      numPerturbations_(minPerturbations)
{
    if (minPerturbations > maxPerturbations)
        throw std::invalid_argument(
            "min_perturbations must be <= max_perturbations.");
}

size_t PerturbationManager::numPerturbations() const
{
    return numPerturbations_;
}

void PerturbationManager::shuffle(RandomNumberGenerator &rng)
{
    auto const range = maxPerturbations_ - minPerturbations_;
    numPerturbations_ = minPerturbations_ + rng.randint(range + 1);
}
