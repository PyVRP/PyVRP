#ifndef PYVRP_SEARCH_NEIGHBOURHOOD_H
#define PYVRP_SEARCH_NEIGHBOURHOOD_H

#include "ProblemData.h"

#include <vector>

namespace pyvrp::search
{
/**
 * NeighbourhoodParams(
 *    weight_wait_time: float = 0.2,
 *    num_neighbours: int = 50,
 *    symmetric_proximity: bool = True,
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
 *     Number of other clients that are in each client's granular
 *     neighbourhood. This parameter determines the size of the overall
 *     neighbourhood.
 * symmetric_proximity
 *     Whether to calculate a symmetric proximity matrix. This ensures edge
 *     :math:`(i, j)` is given the same weight as :math:`(j, i)`.
 *
 * Raises
 * ------
 * ValueError
 *     When ``num_neighbours`` is not strictly positive.
 */
struct NeighbourhoodParams
{
    double const weightWaitTime;
    size_t const numNeighbours;
    bool const symmetricProximity;

    NeighbourhoodParams(double weightWaitTime = 0.2,
                        size_t numNeighbours = 50,
                        bool symmetricProximity = true);

    bool operator==(NeighbourhoodParams const &other) const = default;
};

/**
 * Computes neighbours defining the neighbourhood for a problem instance.
 */
std::vector<std::vector<size_t>>
computeNeighbours(ProblemData const &data, NeighbourhoodParams const &params);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_NEIGHBOURHOOD_H
