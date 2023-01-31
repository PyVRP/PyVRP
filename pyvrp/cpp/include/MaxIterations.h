#ifndef MAXITERATIONS_H
#define MAXITERATIONS_H

#include "StoppingCriterion.h"

#include <stdexcept>

/**
 * Stopping criterion that stops after a given number of iterations.
 */
class MaxIterations : public StoppingCriterion
{
    size_t const maxIters;
    size_t currIters = 0;

public:
    bool operator()(size_t const bestCost) override
    {
        return maxIters < ++currIters;
    }

    explicit MaxIterations(size_t const maxIterations) : maxIters(maxIterations)
    {
        if (maxIterations == 0)
            throw std::invalid_argument("Zero iterations is not understood.");
    }
};

#endif  // MAXITERATIONS_H
