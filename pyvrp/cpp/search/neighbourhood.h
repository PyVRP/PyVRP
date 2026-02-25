#ifndef PYVRP_SEARCH_NEIGHBOURHOOD_H
#define PYVRP_SEARCH_NEIGHBOURHOOD_H

#include "ProblemData.h"

#include <vector>

namespace pyvrp::search
{
/**
 * NeighbourhoodParams(
 *    weight_wait_time: float = 0.2,
 *    weight_time_warp: float = 1.0,
 *    num_neighbours: int = 50,
 *    symmetric_proximity: bool = True,
 *    symmetric_neighbours: bool = False,
 * )
 *
 * Configuration for calculating a granular neighbourhood.
 *
 * Attributes
 * ----------
 * weight_wait_time
 *     Penalty weight given to the minimum wait time aspect of the proximity
 *     calculation. A large wait time indicates the clients are far apart
 *     in duration/time.
 * weight_time_warp
 *     Penalty weight given to the minimum time warp aspect of the proximity
 *     calculation. A large time warp indicates the clients are far apart in
 *     duration/time.
 * num_neighbours
 *     Number of other clients that are in each client's granular
 *     neighbourhood. This parameter determines the size of the overall
 *     neighbourhood.
 * symmetric_proximity
 *     Whether to calculate a symmetric proximity matrix. This ensures edge
 *     :math:`(i, j)` is given the same weight as :math:`(j, i)`.
 * symmetric_neighbours
 *     Whether to symmetrise the neighbourhood structure. This ensures that
 *     when edge :math:`(i, j)` is in, then so is :math:`(j, i)`. Note that
 *     this is *not* the same as ``symmetric_proximity``.
 *
 * Raises
 * ------
 * ValueError
 *     When ``num_neighbours`` is not strictly positive.
 */
struct NeighbourhoodParams
{
    double const weightWaitTime;
    double const weightTimeWarp;
    size_t const numNeighbours;
    bool const symmetricProximity;
    bool const symmetricNeighbours;

    NeighbourhoodParams(double weightWaitTime = 0.2,
                        double weightTimeWarp = 1.0,
                        size_t numNeighbours = 50,
                        bool symmetricProximity = true,
                        bool symmetricNeighbours = false);

    bool operator==(NeighbourhoodParams const &other) const = default;
};

/**
 * Computes neighbours defining the neighbourhood for a problem instance.
 */
std::vector<std::vector<size_t>>
computeNeighbours(ProblemData const &data, NeighbourhoodParams const &params);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_NEIGHBOURHOOD_H
