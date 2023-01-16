#include "Result.h"

Individual const &Result::getBestFound() const { return bestFound; }

Statistics const &Result::getStatistics() const { return stats; }

size_t Result::getIterations() const { return numIters; }

double Result::getRunTime() const { return runTime; }
