#ifndef RESULT_H
#define RESULT_H

#include "Individual.h"
#include "Statistics.h"

#include <utility>
#include <vector>

class Result
{
    Individual const bestFound;
    Statistics const stats;
    size_t numIters;
    double runTime;

public:
    Result(Individual const &bestFound,
           Statistics stats,
           size_t numIters,
           double runTime)
        : bestFound(bestFound),
          stats(std::move(stats)),
          numIters(numIters),
          runTime(runTime)
    {
    }

    /**
     * Returns the best observed solution.
     */
    [[nodiscard]] Individual const &getBestFound() const { return bestFound; }

    /**
     * Returns statistics collected by the genetic algorithm.
     */
    [[nodiscard]] Statistics const &getStatistics() const { return stats; }

    /**
     * Returns the total number of iterations.
     */
    [[nodiscard]] size_t getIterations() const { return numIters; }

    /**
     * Returns the total elapsed runtime in seconds.
     */
    [[nodiscard]] double getRunTime() const { return runTime; }
};

#endif  // RESULT_H
