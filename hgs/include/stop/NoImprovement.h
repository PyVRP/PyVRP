#ifndef HGS_NOIMPROVEMENT_H
#define HGS_NOIMPROVEMENT_H

#include "StoppingCriterion.h"

class NoImprovement : public StoppingCriterion
{
    size_t const maxIters;
    size_t target = INT_MAX;
    size_t currIters = 0;

public:
    bool operator()(Individual const &best) override
    {
        if (best.cost() < target)
        {
            target = best.cost();
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
