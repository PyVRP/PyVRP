#ifndef PYVRP_SEARCH_SEARCHSPACE_H
#define PYVRP_SEARCH_SEARCHSPACE_H

#include "Activity.h"
#include "DynamicBitset.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Route.h"

#include <map>
#include <vector>

namespace pyvrp::search
{
/**
 * SearchSpace(data: ProblemData, neighbours: dict[Activity, list[Activity]])
 *
 * Manages a search space for the local search. The search space is granular,
 * around the given neighbourhood, and uses the concept of promising activities
 * to determine which activity's neighbourhoods to search. It can also be used
 * to define a (randomised) search ordering for activities, routes, and vehicle
 * types.
 */
class SearchSpace
{
public:
    using Neighbours = std::map<Activity, std::vector<Activity>>;

private:
    // Neighbourhood restrictions: list of nearby clients for each client.
    Neighbours neighbours_;

    // Tracks clients and shipments that can likely be improved by local search
    // operators.
    DynamicBitset promising_;

    // Activity order used for node-based search.
    std::vector<Activity> activityOrder_;

    // Vehicle type order - pairs of [veh type, offset] - used for empty route
    // search.
    std::vector<std::pair<size_t, size_t>> vehTypeOrder_;

public:
    SearchSpace(ProblemData const &data, Neighbours neighbours);

    /**
     * Set the neighbourhood structure of this search space.
     */
    void setNeighbours(Neighbours neighbours);

    /**
     * Returns the current neighbourhood structure.
     */
    Neighbours const &neighbours() const;

    /**
     * Returns the vector of neighbours for a given client or shipment
     * activity.
     */
    std::vector<Activity> const &neighboursOf(Activity const &activity) const;

    /**
     * Returns whether the given activity is a promising evaluation candidate.
     */
    bool isPromising(Activity const &activity) const;

    /**
     * Marks the given client as promising.
     */
    void markPromising(Activity const &activity);

    /**
     * Convenient overload for route nodes. Since this is typically used during
     * insert and removals, this method marks the given node and its direct
     * neighbours as promising. The node must currently be in a route.
     *
     * Does not mark depots.
     */
    void markPromising(Route::Node const *node);

    /**
     * Marks all clients and shipments as promising.
     */
    void markAllPromising();

    /**
     * Unmarks all clients and shipments as promising.
     */
    void unmarkAllPromising();

    /**
     * Returns a randomised order in which the client and shipment search space
     * may be traversed. This order remains unchanged until :meth:`~shuffle` is
     * called.
     */
    std::vector<Activity> const &activityOrder() const;

    /**
     * Returns a randomised order in which the vehicle type space may be
     * traversed. This order remains unchanged until :meth:`~shuffle` is called.
     */
    std::vector<std::pair<size_t, size_t>> const &vehTypeOrder() const;

    /**
     * Randomises the activity, route, and vehicle type orders using the given
     * random number generator.
     */
    void shuffle(RandomNumberGenerator &rng);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SEARCHSPACE_H
