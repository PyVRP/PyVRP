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
    [[nodiscard]] Individual const &getBestFound() const;

    /**
     * Returns statistics collected by the genetic algorithm.
     */
    [[nodiscard]] Statistics const &getStatistics() const;

    /**
     * Returns the total number of iterations.
     */
    [[nodiscard]] size_t getIterations() const;

    /**
     * Returns the total elapsed runtime in seconds.
     */
    [[nodiscard]] double getRunTime() const;
};

#endif  // RESULT_H
