#ifndef PYVRP_ORDERED_CROSSOVER_H
#define PYVRP_ORDERED_CROSSOVER_H

#include "CostEvaluator.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Solution.h"

#include <utility>

namespace pyvrp::crossover
{
/**
 * Performs an ordered crossover (OX) operation between the two given parents.
 * The clients between the [start, end) indices from the first route are copied
 * into a new solution, and any missing clients that are present in the second
 * solution are then copied in as well.
 *
 * @param parents   The parent solutions.
 * @param data      The problem data.
 * @param indices   Tuple of [start, end) indices of the first route to use.
 * @return A new offspring.
 */
// The above is an internal docstring: the OX operator is wrapped on the Python
// side and also documented there.
Solution
orderedCrossover(std::pair<Solution const *, Solution const *> const &parents,
                 ProblemData const &data,
                 std::pair<size_t, size_t> const &indices);
}  // namespace pyvrp::crossover

#endif  // PYVRP_ORDERED_CROSSOVER_H
