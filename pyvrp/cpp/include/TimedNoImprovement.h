#ifndef HGS_TIMEDNOIMPROVEMENT_H
#define HGS_TIMEDNOIMPROVEMENT_H

#include "MaxRuntime.h"
#include "NoImprovement.h"

/**
 * Stopping criterion that stops after a given number of iterations without
 * improvement, or after a fixed runtime (whichever happens first).
 */
class TimedNoImprovement : public StoppingCriterion
{
    NoImprovement noImprovement;
    MaxRuntime maxRuntime;

public:
    bool operator()(size_t const bestCost) override
    {
        return noImprovement(bestCost) || maxRuntime(bestCost);
    }

    TimedNoImprovement(size_t const maxIterations, double const maxRuntime)
        : noImprovement(maxIterations), maxRuntime(maxRuntime)
    {
    }
};

#endif  // HGS_TIMEDNOIMPROVEMENT_H
