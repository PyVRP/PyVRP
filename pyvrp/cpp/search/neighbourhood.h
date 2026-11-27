#ifndef PYVRP_SEARCH_NEIGHBOURHOOD_H
#define PYVRP_SEARCH_NEIGHBOURHOOD_H

#include "Activity.h"
#include "ProblemData.h"

#include <unordered_map>
#include <vector>

namespace pyvrp::search
{
/**
 * NeighbourhoodParams(
 *    weight_wait_time: float = 0.2,
 *    num_neighbours: int = 60,
 *    symmetric_proximity: bool = True,
 *    dynamic: bool = True,
 *    dynamic_patience: int = 1000,
 * )
 *
 * Configuration for calculating a granular neighbourhood.
 *
 * Parameters
 * ----------
 * weight_wait_time
 *     Penalty weight given to the minimum wait time aspect of the proximity
 *     calculation. A large wait time indicates the clients are far apart
 *     in duration/time.
 * num_neighbours
 *     Maximum number of activities in each granular neighbourhood. The full
 *     list is used by perturbation and insertion. Dynamic local search selects
 *     prefixes of quarter, half, and full size (15, 30, and 60 by default).
 * symmetric_proximity
 *     Whether to calculate a symmetric proximity matrix. This ensures edge
 *     :math:`(i, j)` is given the same weight as :math:`(j, i)`.
 * dynamic
 *     Whether the solver adapts local search neighbourhoods from quarter to
 *     half to full size. Perturbation and insertion use the full list.
 * dynamic_patience
 *     Number of unsuccessful participations before widening a neighbourhood.
 *     Must be positive.
 *
 * Raises
 * ------
 * ValueError
 *     When ``num_neighbours`` or ``dynamic_patience`` is not strictly positive.
 */
struct NeighbourhoodParams
{
    double const weightWaitTime;
    size_t const numNeighbours;
    bool const symmetricProximity;
    bool const dynamic;
    size_t const dynamicPatience;

    NeighbourhoodParams(double weightWaitTime = 0.2,
                        size_t numNeighbours = 60,
                        bool symmetricProximity = true,
                        bool dynamic = true,
                        size_t dynamicPatience = 1000);

    bool operator==(NeighbourhoodParams const &other) const = default;
};

/**
 * Computes neighbours defining the neighbourhood for a problem instance.
 */
std::unordered_map<Activity, std::vector<Activity>>
computeNeighbours(ProblemData const &data, NeighbourhoodParams const &params);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_NEIGHBOURHOOD_H
