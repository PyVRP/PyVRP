#ifndef PYVRP_SEARCH_NEIGHBOURHOOD_H
#define PYVRP_SEARCH_NEIGHBOURHOOD_H

#include "Activity.h"
#include "ProblemData.h"

#include <unordered_map>
#include <vector>

namespace pyvrp::search
{
/**
 * NeighbourhoodParams(weight_wait_time: float = 0.2, num_neighbours: int = 50)
 *
 * Configuration for calculating a granular neighbourhood.
 * Candidates V in U's neighbourhood are ranked by the cost of inserting
 * the arc V -> U, minus V's opportunity cost. The opportunity cost is the
 * mean of up to ten cheapest time-feasible outgoing arcs from V, excluding
 * V itself and depots. It is zero if no such arcs exist. Shipment
 * neighbourhoods rank predecessors of the pickup activity.
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

    NeighbourhoodParams(double weightWaitTime = 0.2, size_t numNeighbours = 50);

    bool operator==(NeighbourhoodParams const &other) const = default;
};

/**
 * Computes neighbours defining the neighbourhood for a problem instance.
 */
std::unordered_map<Activity, std::vector<Activity>>
computeNeighbours(ProblemData const &data, NeighbourhoodParams const &params);
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_NEIGHBOURHOOD_H
