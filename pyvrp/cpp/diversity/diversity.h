#ifndef HGS_DIVERSITY_H
#define HGS_DIVERSITY_H

#include "Individual.h"
#include "ProblemData.h"

#include <functional>

typedef std::function<double(
    ProblemData const &, Individual const &, Individual const &)>
    DiversityMeasure;

/**
 * Computes a diversity distance between the two given individuals, based on
 * the number of arcs that differ between them. The distance is normalised to
 * [0, 1].
 *
 * @param data   Data instance describing the problem that is being solved.
 * @param first  First individual.
 * @param second Second individual.
 * @return The (symmetric) broken pairs distance between the two
 *         individuals.
 */
double brokenPairsDistance(ProblemData const &data,
                           Individual const &first,
                           Individual const &second);

#endif  // HGS_DIVERSITY_H
