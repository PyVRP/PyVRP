#include "Result.h"

[[nodiscard]] Individual const &Result::getBestFound() const
{
    return bestFound;
}

[[nodiscard]] Statistics const &Result::getStatistics() const { return stats; }

[[nodiscard]] size_t Result::getIterations() const { return numIters; }

[[nodiscard]] double Result::getRunTime() const { return runTime; }
