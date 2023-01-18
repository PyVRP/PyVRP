#include "Result.h"

Result::Result(Individual const &bestFound,
               Statistics stats,
               size_t numIters,
               double runTime)
    : bestFound(bestFound),
      stats(std::move(stats)),
      numIters(numIters),
      runTime(runTime)
{
}

Individual const &Result::getBestFound() const { return bestFound; }

Statistics const &Result::getStatistics() const { return stats; }

size_t Result::getIterations() const { return numIters; }

double Result::getRunTime() const { return runTime; }
