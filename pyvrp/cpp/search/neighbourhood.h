#ifndef PYVRP_SEARCH_NEIGHBOURHOOD_H
#define PYVRP_SEARCH_NEIGHBOURHOOD_H

#include "Activity.h"
#include "ProblemData.h"

#include <vector>

namespace pyvrp::search
{
/**
 * TODO
 */
class Neighbourhood
{
    std::vector<std::vector<Activity>> clients_;
    std::vector<std::vector<Activity>> pickups_;

public:
    Neighbourhood() = default;
    Neighbourhood(Neighbourhood &&) = default;
    Neighbourhood(Neighbourhood const &) = default;

    Neighbourhood &operator=(Neighbourhood const &) = default;
    Neighbourhood &operator=(Neighbourhood &&) = default;

    Neighbourhood(std::vector<std::vector<Activity>> clients,
                  std::vector<std::vector<Activity>> pickups);

    std::vector<std::vector<Activity>> const &clients() const;
    std::vector<std::vector<Activity>> const &pickups() const;

    std::vector<Activity> const &operator[](Activity const &activity) const;

    size_t size() const;
};

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
 *     Number of activities in each granular neighbourhood. This parameter
 *     determines the size of the overall neighbourhood.
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
Neighbourhood computeNeighbours(ProblemData const &data,
                                NeighbourhoodParams const &params);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_NEIGHBOURHOOD_H
