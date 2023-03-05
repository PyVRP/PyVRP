#ifndef CROSSOVER_H
#define CROSSOVER_H

#include "Individual.h"
#include "PenaltyManager.h"
#include "ProblemData.h"
#include "XorShift128.h"

#include <functional>
#include <vector>

namespace crossover
{
/**
 * Greedily inserts the unplanned clients into non-empty routes.
 */
void greedyRepair(std::vector<std::vector<int>> &routes,
                  std::vector<int> const &unplanned,
                  ProblemData const &data);
}  // namespace crossover

typedef std::function<Individual(
    std::pair<Individual const *, Individual const *> const &,
    ProblemData const &,
    PenaltyManager &,
    XorShift128 &)>
    CrossoverOperator;

/**
 * Performs two SREX crossovers of the given parents. # TODO
 * <br />
 * Yuichi Nagata and Shigenobu Kobayashi. "A memetic algorithm for the pickup
 * and delivery problem with time windows using selective route exchange
 * crossover". In: International Conference on Parallel Problem Solving from
 * Nature. Springer. 2010, pp. 536–545.
 */
Individual selectiveRouteExchange(
    std::pair<Individual const *, Individual const *> const &parents,
    ProblemData const &data,
    PenaltyManager const &penaltyManager,
    size_t startA,
    size_t startB,
    size_t const nMovedRoutes);

#endif  // CROSSOVER_H
