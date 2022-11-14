#ifndef MAXITERATIONS_H
#define MAXITERATIONS_H

#include "StoppingCriterion.h"

class MaxIterations : public StoppingCriterion
{
    size_t const maxIters;
    size_t currIters = 0;

public:
    bool operator()() override { return maxIters < ++currIters; }

    explicit MaxIterations(size_t const maxIterations) : maxIters(maxIterations)
    {
        if (maxIterations == 0)
            throw std::runtime_error("Zero iterations is not understood.");
    }
};

#endif  // MAXITERATIONS_H
