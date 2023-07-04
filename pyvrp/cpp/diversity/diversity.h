#ifndef PYVRP_DIVERSITY_H
#define PYVRP_DIVERSITY_H

#include "ProblemData.h"
#include "Solution.h"

#include <functional>

typedef std::function<double(Solution const &, Solution const &)>
    DiversityMeasure;

/**
 * Computes a diversity distance between the two given solutions, based on
 * the number of arcs that differ between them. The distance is normalised to
 * [0, 1].
 *
 * @param first  First solution.
 * @param second Second solution.
 * @return The (symmetric) broken pairs distance between the two solutions.
 */
double brokenPairsDistance(Solution const &first, Solution const &second);

#endif  // PYVRP_DIVERSITY_H
