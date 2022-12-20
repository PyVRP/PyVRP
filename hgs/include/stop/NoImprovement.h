#ifndef HGS_NOIMPROVEMENT_H
#define HGS_NOIMPROVEMENT_H

#include "StoppingCriterion.h"

#include <climits>

class NoImprovement : public StoppingCriterion
{
    size_t const maxIters;
    size_t target = INT_MAX;
    size_t currIters = 0;

public:
    bool operator()(size_t const bestCost) override
    {
        if (bestCost < target)
        {
            target = bestCost;
            currIters = 0;
            return false;
        }

        return maxIters < ++currIters;
    }

    explicit NoImprovement(size_t const maxIterations) : maxIters(maxIterations)
    {
        if (maxIterations == 0)
            throw std::runtime_error("Zero iterations is not understood.");
    }
};

#endif  // HGS_NOIMPROVEMENT_H
