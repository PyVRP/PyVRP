#ifndef PYVRP_SEARCH_SEARCHSPACE_H
#define PYVRP_SEARCH_SEARCHSPACE_H

#include "Activity.h"
#include "DynamicBitset.h"
#include "DynamicNeighbourhood.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Route.h"

#include <memory>
#include <span>
#include <unordered_map>
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
 * types, and tracks evaluation history to skip unchanged route pairs.
 */
class SearchSpace
{
public:
    using Neighbours = std::unordered_map<Activity, std::vector<Activity>>;

private:
    // Neighbourhood restrictions: list of nearby activities for each client
    // and pickup activity.
    Neighbours neighbours_;

    // Activity order used for node-based search.
    std::vector<Activity> activityOrder_;

    // Vehicle type order - pairs of [veh type, offset] - used for empty route
    // search.
    std::vector<std::pair<size_t, size_t>> vehTypeOrder_;

    // Tracks clients and shipments that can likely be improved by local search
    // operators. Clients are tracked first, followed by shipments.
    DynamicBitset promising_;
    size_t const numClients_;
    size_t const numShipments_;
    std::shared_ptr<DynamicNeighbourhood> dynamic_;

    // Evaluation and modification counters for the current local search.
    std::vector<int> lastTest_;
    std::vector<int> lastUpdate_;
    size_t numUpdates_ = 0;
    std::span<Route const> routes_;

    size_t index(Activity const &activity) const;

    void touch(Activity const &activity);

public:
    SearchSpace(ProblemData const &data, Neighbours neighbours);

    /**
     * Resets evaluation history for a new local search over these routes.
     * The routes must remain valid until the next reset.
     */
    void reset(std::span<Route const> routes);

    /** Records a test and returns the activity's previous test counter. */
    int test(Activity const &activity);

    /** Returns whether either route changed since the given test counter. */
    bool updatedSince(Route const *U, Route const *V, int lastTest) const;

    /** Counts one solution modification and marks its affected routes. */
    void update(Route const *U, Route const *V);

    /** Returns the number of modifications in the current local search. */
    size_t numUpdates() const;

    std::shared_ptr<DynamicNeighbourhood>
    configureDynamicNeighbourhood(Neighbours const &candidates,
                                  size_t patience
                                  = DynamicNeighbourhood::DEFAULT_PATIENCE)
    {
        dynamic_ = std::make_shared<DynamicNeighbourhood>(
            candidates, numClients_, numShipments_, patience);
        return dynamic_;
    }

    /** Records participation in an applied binary move, in argument order. */
    void touch(Route::Node const *U, Route::Node const *V);

    Neighbours activeNeighbours() const
    {
        return dynamic_ ? dynamic_->neighbours() : neighbours_;
    }

    std::vector<Activity> const &
    activeNeighboursOf(Activity const &activity) const
    {
        return dynamic_ ? dynamic_->neighboursOf(activity)
                        : neighboursOf(activity);
    }

    /**
     * Set the neighbourhood structure of this search space. For each client
     * and pickup activity, the neighbourhood structure is a vector of nearby
     * clients, pickups, and deliveries.
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
     * Marks the given activity as promising.
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
