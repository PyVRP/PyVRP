#ifndef PYVRP_SEARCH_ROUTE_H
#define PYVRP_SEARCH_ROUTE_H

#include "DistanceSegment.h"
#include "DurationSegment.h"
#include "LoadSegment.h"
#include "ProblemData.h"

#include <algorithm>
#include <cassert>
#include <iosfwd>
#include <numeric>

namespace pyvrp::search
{
/**
 * This ``Route`` class supports fast delta cost computations and in-place
 * modification. It can be used to implement move evaluations.
 *
 * A ``Route`` object tracks a full route, including the depots. A route
 * consists of trips, where every trip starts and ends with a depot. For
 * example, a route consisting of 2 trips will have 4 depot nodes in total
 * (start/end depot for first trip and start/end depot for second trip). The
 * clients and depots on the route can be accessed using ``Route::operator[]``
 * on a ``route`` object: ``route[0]`` and ``route[route.nodes.size() - 1]``
 * are the start depot of the first trip and end depot of the last trip,
 * respectively, and the clients and remaining depot nodes are on the indices
 * in between. Note that ``Route::size()`` returns the number of *clients* in
 * the route; this excludes the depots.
 *
 * .. note::
 *
 *    Modifications to the ``Route`` object do not immediately propagate to its
 *    statistics like time window, load and distance data. To make that happen,
 *    ``Route::update()`` must be called!
 */
class Route
{
public:
    /**
     * A simple class that tracks a new proposed structure for a given route.
     * This new structure can be efficiently evaluated by calling appropriate
     * member functions to return concatenation schemes that detail the
     * newly proposed route's statistics.
     */
    template <typename... Segments> class Proposal
    {
        Route const *current;
        ProblemData const &data;
        std::tuple<Segments...> segments;

    public:
        Proposal(Route const *current,
                 ProblemData const &data,
                 Segments &&...segments);

        Route const *route() const;

        DistanceSegment distanceSegment() const;
        DurationSegment durationSegment() const;
        Load excessLoad(size_t dimension) const;
    };

    /**
     * Light wrapper class around a client or depot location. This class tracks
     * the route it is in, and the position and role it currently has in that
     * route.
     */
    class Node
    {
    public:
        enum class NodeType
        {
            StartDepot,
            EndDepot,
            Client
        };

    private:
        friend class Route;

        size_t loc_;      // Location represented by this node
        size_t idx_;      // Position in the route
        size_t tripIdx_;  // Trip index at this node
        Route *route_;    // Indicates membership of a route, if any
        NodeType type_;   // Type of the node

    public:
        Node(size_t loc, NodeType type = NodeType::Client);

        /**
         * Returns the location represented by this node.
         */
        [[nodiscard]] inline size_t client() const;  // TODO rename to loc

        /**
         * Returns this node's position in a route. This value is ``0`` when
         * the node is *not* in a route.
         */
        [[nodiscard]] inline size_t idx() const;

        /**
         * Returns the trip index at this node. Note that the value ``0`` can
         * either mean that the node is *not* in a route, or that it is in the
         * first trip of a route.
         */
        [[nodiscard]] inline size_t tripIdx() const;

        /**
         * Returns the route this node is currently in. If the node is not in
         * a route, this returns ``None`` (C++: ``nullptr``).
         */
        [[nodiscard]] inline Route *route() const;

        /**
         * Returns the type of this node.
         */
        [[nodiscard]] inline NodeType type() const;

        /**
         * Returns whether this node is a depot. A node can only be a depot if
         * it is in a route.
         */
        [[nodiscard]] inline bool isDepot() const;

        /**
         * Returns whether this node is a client.
         */
        [[nodiscard]] inline bool isClient() const;

        /**
         * Returns whether this node is a start depot node.
         */
        [[nodiscard]] inline bool isStartDepot() const;

        /**
         * Returns whether this node is an end depot node.
         */
        [[nodiscard]] inline bool isEndDepot() const;

        /**
         * Assigns the node to the given route, at the given position index and
         * trip index.
         */
        void assign(Route *route, size_t idx, size_t tripIdx);

        /**
         * Removes the node from its assigned route, if any.
         */
        void unassign();
    };

private:
    using LoadSegments = std::vector<LoadSegment>;

    /**
     * Class storing data related to the route segment starting at ``start``,
     * and ending at the depot (inclusive).
     */
    class SegmentAfter
    {
        Route const &route;
        size_t const start;

    public:
        inline size_t first() const;     // client at start
        inline size_t last() const;      // end depot
        inline bool endsTrip() const;    // true, since last is end depot node
        inline size_t numTrips() const;  // number of trips

        inline SegmentAfter(Route const &route, size_t start);
        inline DistanceSegment distance(size_t profile) const;
        inline DurationSegment duration(size_t profile) const;
        inline LoadSegment const &load(size_t dimension, size_t trip) const;
    };

    /**
     * Class storing data related to the route segment starting at the depot,
     * and ending at ``end`` (inclusive).
     */
    class SegmentBefore
    {
        Route const &route;
        size_t const end;

    public:
        inline size_t first() const;     // start depot
        inline size_t last() const;      // depot/client at end
        inline bool endsTrip() const;    // whether last is end depot node
        inline size_t numTrips() const;  // number of trips

        inline SegmentBefore(Route const &route, size_t end);
        inline DistanceSegment distance(size_t profile) const;
        inline DurationSegment duration(size_t profile) const;
        inline LoadSegment const &load(size_t dimension, size_t trip) const;
    };

    /**
     * Class storing data related to the route segment starting at ``start``,
     * and ending at ``end`` (inclusive).
     */
    class SegmentBetween
    {
        Route const &route;
        size_t const start;
        size_t const end;

    public:
        inline size_t first() const;     // depot/client at start
        inline size_t last() const;      // depot/client at end
        inline bool endsTrip() const;    // whether last is end depot node
        inline size_t numTrips() const;  // number of trips

        inline SegmentBetween(Route const &route, size_t start, size_t end);
        inline DistanceSegment distance(size_t profile) const;
        inline DurationSegment duration(size_t profile) const;
        inline LoadSegment load(size_t dimension, size_t trip) const;
    };

    /**
     * Class storing data related to the start depot of the route's vehicle
     * type. This class should be used when inserting a new trip in the middle
     * or at the end of the route, so not for the first trip in the route.
     */
    class SegmentStartDepot
    {
        Route const &route;

    public:
        inline size_t first() const;     // client at start
        inline size_t last() const;      // client at end
        inline bool endsTrip() const;    // false
        inline size_t numTrips() const;  // number of trips

        inline SegmentStartDepot(Route const &route);
        inline DistanceSegment distance(size_t profile) const;
        inline DurationSegment duration(size_t profile) const;
        inline LoadSegment load(size_t dimension, size_t trip) const;
    };

    /**
     * Class storing data related to the end depot of the route's vehicle type.
     * This class should be used when inserting a new trip in the middle or at
     * the end of the route, so not for the first trip in the route.
     */
    class SegmentEndDepot
    {
        Route const &route;

    public:
        inline size_t first() const;     // client at start
        inline size_t last() const;      // client at end
        inline bool endsTrip() const;    // true
        inline size_t numTrips() const;  // number of trips

        inline SegmentEndDepot(Route const &route);
        inline DistanceSegment distance(size_t profile) const;
        inline DurationSegment duration(size_t profile) const;
        inline LoadSegment load(size_t dimension, size_t trip) const;
    };

    ProblemData const &data;

    ProblemData::VehicleType const &vehicleType_;
    size_t const idx_;

    std::vector<std::pair<Node, Node>>
        depotNodes;  // Every trip has departure and return depot node pair.

    std::vector<Node *> nodes;   // Nodes in this route, including depots
    std::vector<size_t> visits;  // Locations in this route, incl. depots
    std::pair<double, double> centroid_;  // Center point of route's clients

    std::vector<Distance> cumDist;  // Dist of depot -> client (incl.)

    // Load data, for each load dimension. These vectors form matrices, where
    // the rows index the load dimension, and the columns the nodes.
    std::vector<LoadSegments> loadAt;          // Load data at each node
    std::vector<LoadSegments> tripLoadAfter;   // Load of client -> depot (incl)
    std::vector<LoadSegments> tripLoadBefore;  // Load of depot -> client (incl)

    // Load per trip, for each load dimension. This vector forms a matrix,
    // where the rows index the load dimension, and the columns the trips.
    std::vector<LoadSegments> tripLoad_;

    std::vector<Load> excessLoad_;  // Route excess load (for each dimension)

    std::vector<DurationSegment> durAt;      // Duration data at each node
    std::vector<DurationSegment> durAfter;   // Dur of client -> depot (incl.)
    std::vector<DurationSegment> durBefore;  // Dur of depot -> client (incl.)

#ifndef NDEBUG
    // When debug assertions are enabled, we use this flag to check whether
    // the statistics are still in sync with the route's nodes list. Statistics
    // are only updated after calling ``update()``. If that function has not
    // yet been called after inserting or removing nodes, this flag is active,
    // and asserts on statistics getters will fail.
    bool dirty = false;
#endif

public:
    /**
     * Route index.
     */
    [[nodiscard]] inline size_t idx() const;

    /**
     * @return The client or depot node at the given ``idx``.
     */
    [[nodiscard]] inline Node *operator[](size_t idx);

    // First depot in the route.
    [[nodiscard]] std::vector<Node *>::const_iterator begin() const;
    [[nodiscard]] std::vector<Node *>::iterator begin();

    // One element past the end depot.
    [[nodiscard]] std::vector<Node *>::const_iterator end() const;
    [[nodiscard]] std::vector<Node *>::iterator end();

    /**
     * Returns a route proposal object that stores the given route segment
     * arguments.
     */
    template <typename... Segments>
    [[nodiscard]] Proposal<Segments...> proposal(Segments &&...segments) const;

    /**
     * Tests if this route is feasible.
     *
     * @return true if the route is feasible, false otherwise.
     */
    [[nodiscard]] inline bool isFeasible() const;

    /**
     * Determines whether this route is load-feasible.
     *
     * @return true if the route exceeds the capacity, false otherwise.
     */
    [[nodiscard]] inline bool hasExcessLoad() const;

    /**
     * Determines whether this route is distance-feasible.
     *
     * @return true if the route exceeds the maximum distance constraint, false
     *         otherwise.
     */
    [[nodiscard]] inline bool hasExcessDistance() const;

    /**
     * Determines whether this route is time-feasible.
     *
     * @return true if the route has time warp, false otherwise.
     */
    [[nodiscard]] inline bool hasTimeWarp() const;

    /**
     * @return The total load for the given load dimension and given trip on
     * this route.
     */
    [[nodiscard]] inline Load const tripLoad(size_t dim, size_t tripIdx) const;

    /**
     * Total loads on this route.
     */
    [[nodiscard]] inline std::vector<Load> const load() const;

    /**
     * Pickup or delivery loads in excess of the vehicle's capacity.
     */
    [[nodiscard]] inline std::vector<Load> const &excessLoad() const;

    /**
     * Travel distance in excess of the assigned vehicle type's maximum
     * distance constraint.
     */
    [[nodiscard]] inline Distance excessDistance() const;

    /**
     * Capacity of the vehicle servicing this route.
     */
    [[nodiscard]] inline std::vector<Load> const &capacity() const;

    /**
     * @return The location index of this route's starting depot.
     */
    [[nodiscard]] inline size_t startDepot() const;

    /**
     * @return The location index of this route's ending depot.
     */
    [[nodiscard]] inline size_t endDepot() const;

    /**
     * @return The fixed cost of the vehicle servicing this route.
     */
    [[nodiscard]] inline Cost fixedVehicleCost() const;

    /**
     * @return Total distance travelled on this route.
     */
    [[nodiscard]] inline Distance distance() const;

    /**
     * @return Cost of the distance travelled on this route.
     */
    [[nodiscard]] inline Cost distanceCost() const;

    /**
     * @return Cost per unit of distance travelled on this route.
     */
    [[nodiscard]] inline Cost unitDistanceCost() const;

    /**
     * @return The duration of this route.
     */
    [[nodiscard]] inline Duration duration() const;

    /**
     * @return Cost of this route's duration.
     */
    [[nodiscard]] inline Cost durationCost() const;

    /**
     * @return Cost per unit of duration travelled on this route.
     */
    [[nodiscard]] inline Cost unitDurationCost() const;

    /**
     * @return The maximum route duration that the vehicle servicing this route
     *         supports.
     */
    [[nodiscard]] inline Duration maxDuration() const;

    /**
     * @return The maximum route distance that the vehicle servicing this route
     *         supports.
     */
    [[nodiscard]] inline Distance maxDistance() const;

    /**
     * @return Total time warp on this route.
     */
    [[nodiscard]] inline Duration timeWarp() const;

    /**
     * @return The routing profile of the vehicle servicing this route.
     */
    [[nodiscard]] inline size_t profile() const;

    /**
     * @return true if this route is empty, false otherwise.
     */
    [[nodiscard]] inline bool empty() const;

    /**
     * @return Number of nodes (clients + depots) in this route.
     */
    [[nodiscard]] inline size_t size() const;

    /**
     * @return Number of clients in this route.
     */
    [[nodiscard]] inline size_t numClients() const;

    /**
     * @return Number of trips in this route.
     */
    [[nodiscard]] inline size_t numTrips() const;

    /**
     * @return Maximum number of trips that the vehicle servicing this route can
     *         perform.
     */
    [[nodiscard]] inline size_t maxTrips() const;

    /**
     * @return Whether this route contains a depot in the segment given by
     * ``startIndex`` and ``length``.
     */
    [[nodiscard]] inline bool containsDepot(size_t startIdx, size_t length);

    /**
     * Returns an object that can be queried for data associated with the node
     * at idx.
     */
    [[nodiscard]] inline SegmentBetween at(size_t idx) const;

    /**
     * Returns an object that can be queried for data associated with the
     * segment starting at start.
     */
    [[nodiscard]] inline SegmentAfter after(size_t start) const;

    /**
     * Returns an object that can be queried for data associated with the
     * segment ending at end.
     */
    [[nodiscard]] inline SegmentBefore before(size_t end) const;

    /**
     * Returns an object that can be queried for data associated with the
     * segment between [start, end].
     */
    [[nodiscard]] inline SegmentBetween between(size_t start, size_t end) const;

    /**
     * Returns an object that can be queried for data associated with the node
     * corresponding to the start depot.
     */
    [[nodiscard]] inline SegmentStartDepot startDepotSegment() const;

    /**
     * Returns an object that can be queried for data associated with the node
     * corresponding to the end depot.
     */
    [[nodiscard]] inline SegmentEndDepot endDepotSegment() const;

    /**
     * Center point of the client locations on this route.
     */
    [[nodiscard]] std::pair<double, double> const &centroid() const;

    /**
     * @return This route's vehicle type.
     */
    [[nodiscard]] size_t vehicleType() const;

    /**
     * Tests if this route potentially overlaps with the other route, subject
     * to a tolerance in [0, 1].
     */
    [[nodiscard]] bool overlapsWith(Route const &other, double tolerance) const;

    /**
     * Clears all clients on this route. After calling this method, ``empty()``
     * returns true and ``size()`` is zero.
     */
    void clear();

    /**
     * Inserts the given client node before index ``idx``. Assumes the given
     * index is valid.
     */
    void insert(size_t idx, Node *node);

    /**
     * Inserts the given range of client nodes before ``idx``. Assumes the given
     * index is valid.
     */
    template <class InputIt>
    void insert(size_t idx, InputIt first, InputIt last);

    /**
     * Inserts the given client node at the back of the route.
     */
    void push_back(Node *node);

    /**
     * Insert an empty trip. A start/end depot node pair is created and
     * inserted at index ``tripIdx``. Assumes the given index is valid.
     */
    void insertTrip(size_t tripIdx);

    /**
     * Inserts an empty trip at the end of the route. A start/end depot node
     * pair is created and inserted at the back of the route.
     */
    void addTrip();

    /**
     * Removes the client node at ``idx`` from the route.
     */
    void remove(size_t idx);

    /**
     * Removes the trip at ``tripIdx`` from the route. The start/end depot
     * node pair for this trip is removed from the route. Assumes that the trip
     * to be removed is empty.
     */
    void removeTrip(size_t tripIdx);

    /**
     * Swaps the given nodes.
     */
    static void swap(Node *first, Node *second);

    /**
     * Updates this route. To be called after swapping nodes/changing the
     * solution.
     */
    void update();

    Route(ProblemData const &data, size_t idx, size_t vehicleType);
    ~Route();
};

/**
 * Convenience method accessing the node directly before the argument.
 */
inline Route::Node *p(Route::Node *node)
{
    auto &route = *node->route();
    return route[node->idx() - 1];
}

/**
 * Convenience method accessing the node directly after the argument.
 */
inline Route::Node *n(Route::Node *node)
{
    auto &route = *node->route();
    return route[node->idx() + 1];
}

size_t Route::Node::client() const { return loc_; }

size_t Route::Node::idx() const { return idx_; }

size_t Route::Node::tripIdx() const { return tripIdx_; }

Route *Route::Node::route() const { return route_; }

Route::Node::NodeType Route::Node::type() const { return type_; }

bool Route::Node::isDepot() const { return isStartDepot() || isEndDepot(); }

bool Route::Node::isClient() const { return type_ == NodeType::Client; }

bool Route::Node::isStartDepot() const { return type_ == NodeType::StartDepot; }

bool Route::Node::isEndDepot() const { return type_ == NodeType::EndDepot; }

Route::SegmentAfter::SegmentAfter(Route const &route, size_t start)
    : route(route), start(start)
{
    assert(start < route.nodes.size());
}

Route::SegmentBefore::SegmentBefore(Route const &route, size_t end)
    : route(route), end(end)
{
    assert(end < route.nodes.size());
}

Route::SegmentBetween::SegmentBetween(Route const &route,
                                      size_t start,
                                      size_t end)
    : route(route), start(start), end(end)
{
    assert(start <= end && end < route.nodes.size());
}

Route::SegmentStartDepot::SegmentStartDepot(Route const &route) : route(route)
{
}

Route::SegmentEndDepot::SegmentEndDepot(Route const &route) : route(route) {}

DistanceSegment Route::SegmentAfter::distance(size_t profile) const
{
    if (profile == route.profile())
        return {route.cumDist.back() - route.cumDist[start]};

    auto const between = SegmentBetween(route, start, route.nodes.size() - 1);
    return between.distance(profile);
}

DurationSegment Route::SegmentAfter::duration(size_t profile) const
{
    if (profile == route.profile())
        return route.durAfter[start];

    auto const between = SegmentBetween(route, start, route.nodes.size() - 1);
    return between.duration(profile);
}

LoadSegment const &Route::SegmentAfter::load(size_t dimension,
                                             size_t trip) const
{
    assert(trip < numTrips());
    if (trip == 0)  // Load for the first (partial) trip in the segment.
        return route.tripLoadAfter[dimension][start];

    // Complete trip in the segment.
    return route.tripLoad_[dimension][route.nodes[start]->tripIdx() + trip];
}

DistanceSegment Route::SegmentBefore::distance(size_t profile) const
{
    if (profile == route.profile())
        return {route.cumDist[end]};

    auto const between = SegmentBetween(route, 0, end);
    return between.distance(profile);
}

DurationSegment Route::SegmentBefore::duration(size_t profile) const
{
    if (profile == route.profile())
        return route.durBefore[end];

    auto const between = SegmentBetween(route, 0, end);
    return between.duration(profile);
}

LoadSegment const &Route::SegmentBefore::load(size_t dimension,
                                              size_t trip) const
{
    assert(trip < numTrips());
    // Load for the last (partial) trip in the segment.
    if (trip == numTrips() - 1)
        return route.tripLoadBefore[dimension][end];

    // Complete trip in the segment.
    return route.tripLoad_[dimension][trip];
}

size_t Route::SegmentBefore::first() const { return route.visits.front(); }
size_t Route::SegmentBefore::last() const { return route.visits[end]; }
bool Route::SegmentBefore::endsTrip() const
{
    return route.nodes[end]->isEndDepot();
}
size_t Route::SegmentBefore::numTrips() const
{
    return route.nodes[end]->tripIdx() + 1;
}

size_t Route::SegmentAfter::first() const { return route.visits[start]; }
size_t Route::SegmentAfter::last() const { return route.visits.back(); }
bool Route::SegmentAfter::endsTrip() const { return true; }
size_t Route::SegmentAfter::numTrips() const
{
    return route.numTrips() - route.nodes[start]->tripIdx();
}

size_t Route::SegmentBetween::first() const { return route.visits[start]; }
size_t Route::SegmentBetween::last() const { return route.visits[end]; }
bool Route::SegmentBetween::endsTrip() const
{
    return route.nodes[end]->isEndDepot();
}
size_t Route::SegmentBetween::numTrips() const
{
    return route.nodes[end]->tripIdx() - route.nodes[start]->tripIdx() + 1;
}

size_t Route::SegmentStartDepot::first() const { return route.startDepot(); }
size_t Route::SegmentStartDepot::last() const { return route.startDepot(); }
bool Route::SegmentStartDepot::endsTrip() const { return false; }
size_t Route::SegmentStartDepot::numTrips() const { return 1; }

size_t Route::SegmentEndDepot::first() const { return route.endDepot(); }
size_t Route::SegmentEndDepot::last() const { return route.endDepot(); }
bool Route::SegmentEndDepot::endsTrip() const { return true; }
size_t Route::SegmentEndDepot::numTrips() const { return 1; }

DistanceSegment Route::SegmentBetween::distance(size_t profile) const
{
    if (profile != route.profile())  // then we have to compute the distance
    {                                // segment from scratch.
        auto const &mat = route.data.distanceMatrix(profile);
        DistanceSegment distSegment = {0};

        for (size_t step = start; step != end; ++step)
        {
            auto const from = route.visits[step];
            auto const to = route.visits[step + 1];
            distSegment
                = DistanceSegment::merge(mat(from, to), distSegment, {0});
        }

        return distSegment;
    }

    auto const startDist = route.cumDist[start];
    auto const endDist = route.cumDist[end];

    assert(startDist <= endDist);
    return {endDist - startDist};
}

DurationSegment Route::SegmentBetween::duration(size_t profile) const
{
    auto const &mat = route.data.durationMatrix(profile);
    auto durSegment = route.durAt[start];

    for (size_t step = start; step != end; ++step)
    {
        auto const from = route.visits[step];
        auto const to = route.visits[step + 1];
        auto const &durAt = route.durAt[step + 1];
        durSegment = DurationSegment::merge(mat(from, to), durSegment, durAt);
    }

    return durSegment;
}

LoadSegment Route::SegmentBetween::load(size_t dimension, size_t trip) const
{
    assert(trip < numTrips());
    if (numTrips() == 1)  // Single trip in the segment.
    {
        auto const &loads = route.loadAt[dimension];
        auto loadSegment = loads[start];

        for (size_t step = start; step != end; ++step)
            loadSegment = LoadSegment::merge(loadSegment, loads[step + 1]);

        return loadSegment;
    }

    // Multiple trips in the segment.
    if (trip == 0)
        return route.tripLoadAfter[dimension][start];

    if (trip == numTrips() - 1)
        return route.tripLoadBefore[dimension][end];

    return route.tripLoad_[dimension][route.nodes[start]->tripIdx() + trip];
}

DistanceSegment
Route::SegmentStartDepot::distance([[maybe_unused]] size_t profile) const
{
    return {};
}

DurationSegment
Route::SegmentStartDepot::duration([[maybe_unused]] size_t profile) const
{
    // Note that this depot segment should never correspond to the first depot
    // in a route, so the ``twLate`` of the vehicle type must be used for the
    // duration segment instead of the ``startLate`` of the vehicle type.
    return {route.vehicleType_, route.vehicleType_.twLate};
}

LoadSegment Route::SegmentStartDepot::load([[maybe_unused]] size_t dimension,
                                           [[maybe_unused]] size_t trip) const
{
    assert(trip < numTrips());
    return {};
}

DistanceSegment
Route::SegmentEndDepot::distance([[maybe_unused]] size_t profile) const
{
    return {};
}

DurationSegment
Route::SegmentEndDepot::duration([[maybe_unused]] size_t profile) const
{
    return {route.vehicleType_, route.vehicleType_.twLate};
}

LoadSegment Route::SegmentEndDepot::load([[maybe_unused]] size_t dimension,
                                         [[maybe_unused]] size_t trip) const
{
    assert(trip < numTrips());
    return {};
}

bool Route::isFeasible() const
{
    assert(!dirty);
    return !hasExcessLoad() && !hasTimeWarp() && !hasExcessDistance();
}

bool Route::hasExcessLoad() const
{
    assert(!dirty);
    return std::any_of(excessLoad_.begin(),
                       excessLoad_.end(),
                       [](auto const excess) { return excess > 0; });
}

bool Route::hasExcessDistance() const
{
    assert(!dirty);
    return excessDistance() > 0;
}

bool Route::hasTimeWarp() const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return false;
#else
    assert(!dirty);
    return timeWarp() > 0;
#endif
}

size_t Route::idx() const { return idx_; }

Route::Node *Route::operator[](size_t idx)
{
    assert(idx < nodes.size());
    return nodes[idx];
}

template <typename... Segments>
Route::Proposal<Segments...> Route::proposal(Segments &&...segments) const
{
    return {this, data, std::forward<Segments>(segments)...};
}

Load const Route::tripLoad(size_t dim, size_t tripIdx) const
{
    assert(!dirty);
    return tripLoad_[dim][tripIdx].load();
}

std::vector<Load> const Route::load() const
{
    assert(!dirty);
    std::vector<Load> loads(data.numLoadDimensions(), 0);
    for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
        loads[dim] = std::accumulate(tripLoad_[dim].begin(),
                                     tripLoad_[dim].end(),
                                     Load(0),
                                     [](Load load, auto const &loadSegment)
                                     { return load + loadSegment.load(); });

    return loads;
}

std::vector<Load> const &Route::excessLoad() const
{
    assert(!dirty);
    return excessLoad_;
}

Distance Route::excessDistance() const
{
    assert(!dirty);
    return std::max<Distance>(distance() - maxDistance(), 0);
}

std::vector<Load> const &Route::capacity() const
{
    return vehicleType_.capacity;
}

size_t Route::startDepot() const { return vehicleType_.startDepot; }

size_t Route::endDepot() const { return vehicleType_.endDepot; }

Cost Route::fixedVehicleCost() const { return vehicleType_.fixedCost; }

Distance Route::distance() const
{
    assert(!dirty);
    return cumDist.back();
}

Cost Route::distanceCost() const
{
    assert(!dirty);
    return unitDistanceCost() * static_cast<Cost>(distance());
}

Cost Route::unitDistanceCost() const { return vehicleType_.unitDistanceCost; }

Duration Route::duration() const
{
    assert(!dirty);
    return durBefore.back().duration();
}

Cost Route::durationCost() const
{
    assert(!dirty);
    return unitDurationCost() * static_cast<Cost>(duration());
}

Cost Route::unitDurationCost() const { return vehicleType_.unitDurationCost; }

Duration Route::maxDuration() const { return vehicleType_.maxDuration; }

Distance Route::maxDistance() const { return vehicleType_.maxDistance; }

Duration Route::timeWarp() const
{
    assert(!dirty);
    return durBefore.back().timeWarp(maxDuration());
}

size_t Route::profile() const { return vehicleType_.profile; }

bool Route::empty() const { return numClients() == 0; }

size_t Route::size() const { return nodes.size(); }

size_t Route::numClients() const
{
    // Number of clients equals the number of nodes minus the number of depot
    // nodes (2 per trip).
    assert(nodes.size() >= 2 * numTrips());
    return nodes.size() - 2 * numTrips();
}

size_t Route::numTrips() const { return depotNodes.size(); }

size_t Route::maxTrips() const { return vehicleType_.maxTrips; }

bool Route::containsDepot(size_t startIdx, size_t length)
{
    assert(!dirty);
    assert(startIdx + length - 1 < nodes.size());

    // First and last nodes are always depots
    if (startIdx == 0 || startIdx + length >= nodes.size())
        return true;

    // 3 possible cases: depot at start, end or in between.
    // To find depots at the start or end, we compare the trip indices of the
    // node before the start of the segment and the node after the end of the
    // segment.
    return nodes[startIdx - 1]->tripIdx()
           != nodes[startIdx + length]->tripIdx();
}

Route::SegmentBetween Route::at(size_t idx) const
{
    assert(!dirty);
    return {*this, idx, idx};
}

Route::SegmentAfter Route::after(size_t start) const
{
    assert(!dirty);
    return {*this, start};
}

Route::SegmentBefore Route::before(size_t end) const
{
    assert(!dirty);
    return {*this, end};
}

Route::SegmentBetween Route::between(size_t start, size_t end) const
{
    assert(!dirty);
    return {*this, start, end};
}

Route::SegmentStartDepot Route::startDepotSegment() const { return {*this}; }

Route::SegmentEndDepot Route::endDepotSegment() const { return {*this}; }

template <typename... Segments>
Route::Proposal<Segments...>::Proposal(Route const *current,
                                       ProblemData const &data,
                                       Segments &&...segments)
    : current(current),
      data(data),
      segments(std::forward<Segments>(segments)...)
{
}

template <typename... Segments>
Route const *Route::Proposal<Segments...>::route() const
{
    return current;
}

template <typename... Segments>
DistanceSegment Route::Proposal<Segments...>::distanceSegment() const
{
    auto const profile = current->profile();
    auto const &matrix = data.distanceMatrix(profile);

    auto const fn = [&matrix, profile](auto segment, auto &&...args)
    {
        auto distSegment = segment.distance(profile);
        auto last = segment.last();

        auto const merge = [&](auto const &self, auto &&other, auto &&...args)
        {
            distSegment = DistanceSegment::merge(matrix(last, other.first()),
                                                 distSegment,
                                                 other.distance(profile));
            last = other.last();

            if constexpr (sizeof...(args) != 0)
                self(self, std::forward<decltype(args)>(args)...);
        };

        merge(merge, std::forward<decltype(args)>(args)...);
        return distSegment;
    };

    return std::apply(fn, segments);
}

template <typename... Segments>
DurationSegment Route::Proposal<Segments...>::durationSegment() const
{
    auto const profile = current->profile();
    auto const &matrix = data.durationMatrix(profile);

    auto const fn = [&matrix, profile](auto segment, auto &&...args)
    {
        auto durSegment = segment.duration(profile);
        auto last = segment.last();

        auto const merge = [&](auto const &self, auto &&other, auto &&...args)
        {
            durSegment = DurationSegment::merge(matrix(last, other.first()),
                                                durSegment,
                                                other.duration(profile));
            last = other.last();

            if constexpr (sizeof...(args) != 0)
                self(self, std::forward<decltype(args)>(args)...);
        };

        merge(merge, std::forward<decltype(args)>(args)...);
        return durSegment;
    };

    return std::apply(fn, segments);
}

template <typename... Segments>
Load Route::Proposal<Segments...>::excessLoad(size_t dimension) const
{
    auto calculateExcessLoad = [&](LoadSegment const &loadSegment)
    {
        return std::max<Load>(
            loadSegment.load() - current->capacity()[dimension], 0);
    };

    Load excessLoad = 0;
    LoadSegment currentLoadSegment;

    auto processSegment = [&](auto const &segment)
    {
        size_t const segmentTrips = segment.numTrips();
        assert(segmentTrips > 0);

        currentLoadSegment = LoadSegment::merge(currentLoadSegment,
                                                segment.load(dimension, 0));
        if (segmentTrips > 1)
        {
            // End of trip, check if we have excess load.
            excessLoad += calculateExcessLoad(currentLoadSegment);

            // Check excess load for full trips.
            for (size_t idx = 1; idx != segmentTrips - 1; ++idx)
                excessLoad += calculateExcessLoad(segment.load(dimension, idx));

            // Update current load segment.
            currentLoadSegment = segment.load(dimension, segmentTrips - 1);
        }

        // Check if we have excess load for the last trip in the segment.
        if (segment.endsTrip())
        {
            excessLoad += calculateExcessLoad(currentLoadSegment);
            // Next segment (if any) should start with new trip.
            currentLoadSegment = LoadSegment();
        }
    };

    std::apply([&](auto const &...segment) { (processSegment(segment), ...); },
               segments);

    return excessLoad;
}

template <class InputIt>
void Route::insert(size_t idx, InputIt first, InputIt last)
{
    assert(0 < idx && idx < nodes.size());
    // Not allowed to insert between end depot node and start depot node.
    assert(!nodes[idx]->isStartDepot());

    // Insert the nodes before the node currently at position ``idx``.
    size_t tripIdx = nodes[idx]->tripIdx();
    nodes.insert(nodes.begin() + idx, first, last);
    for (auto it = first; it != last; ++it)
    {
        auto node = *it;
        assert(node->isClient());
        node->assign(this, idx++, tripIdx);
    }

    // Note that trip indices do not change when only inserting clients.
    for (size_t after = idx; after != nodes.size(); ++after)
        nodes[after]->idx_ = after;

#ifndef NDEBUG
    dirty = true;
#endif
}
}  // namespace pyvrp::search

// Outputs a route into a given ostream in CVRPLib format
std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route);

#endif  // PYVRP_SEARCH_ROUTE_H
