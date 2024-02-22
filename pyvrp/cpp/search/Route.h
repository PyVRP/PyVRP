#ifndef PYVRP_ROUTE_H
#define PYVRP_ROUTE_H

#include "DurationSegment.h"
#include "LoadSegment.h"
#include "ProblemData.h"

#include <cassert>
#include <iosfwd>

namespace pyvrp::search
{
/**
 * This ``Route`` class supports fast delta cost computations and in-place
 * modification. It can be used to implement move evaluations.
 *
 * A ``Route`` object tracks a full route, including the depots. The clients
 * and depots on the route can be accessed using ``Route::operator[]`` on a
 * ``route`` object: ``route[0]`` and ``route[route.size() + 1]`` are the start
 * and end depots, respectively, and any clients in between are on the indices
 * ``{1, ..., size()}`` (empty if ``size() == 0``). Note that ``Route::size()``
 * returns the number of *clients* in the route; this excludes the depots.
 *
 * .. note::
 *
 *    Modifications to the ``Route`` object do not immediately propagate to its
 *    statistics like time window, load and distance data. To make that happen,
 *    ``Route::update()`` must be called!
 */
class Route
{
    // These proxy classes (defined further below) handle transparent access
    // to the route's segment-specific statistics.
    friend class ProxyAt;
    friend class ProxyAfter;
    friend class ProxyBefore;
    friend class ProxyBetween;

public:
    /**
     * Light wrapper class around a client or depot location. This class tracks
     * the route it is in, and the position and role it currently has in that
     * route.
     */
    class Node
    {
        friend class Route;

        size_t loc_;    // Location represented by this node
        size_t idx_;    // Position in the route
        Route *route_;  // Indicates membership of a route, if any

    public:
        Node(size_t loc);

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
         * Returns the route this node is currently in. If the node is not in
         * a route, this returns ``None`` (C++: ``nullptr``).
         */
        [[nodiscard]] inline Route *route() const;

        /**
         * Returns whether this node is a depot. A node can only be a depot if
         * it is in a route.
         */
        [[nodiscard]] inline bool isDepot() const;
    };

private:
    /**
     * Proxy class for querying data related to a single location in the route,
     * identified by ``idx``.
     */
    class ProxyAt
    {
        Route const *route;
        size_t const idx;

    public:
        inline ProxyAt(Route const &route, size_t idx);
        inline operator LoadSegment const &() const;
        inline operator DurationSegment const &() const;
    };

    /**
     * Proxy class for querying data related to the route segment starting at
     * ``start``, and ending at the depot (inclusive).
     */
    class ProxyAfter
    {
        Route const *route;
        size_t const start;

    public:
        inline ProxyAfter(Route const &route, size_t start);
        inline operator LoadSegment const &() const;
        inline operator DurationSegment const &() const;
    };

    /**
     * Proxy class for querying data related to the route segment starting at
     * the depot, and ending at ``end`` (inclusive).
     */
    class ProxyBefore
    {
        Route const *route;
        size_t const end;

    public:
        inline ProxyBefore(Route const &route, size_t end);
        inline operator LoadSegment const &() const;
        inline operator DurationSegment const &() const;
    };

    /**
     * Proxy class for querying data related to the route segment starting at
     * ``start``, and ending at ``end`` (inclusive).
     */
    class ProxyBetween
    {
        Route const *route;
        size_t const start;
        size_t const end;

    public:
        inline ProxyBetween(Route const &route, size_t start, size_t end);
        inline operator Distance() const;
        inline operator LoadSegment() const;
        inline operator DurationSegment() const;
    };

    ProblemData const &data;

    // Cache the vehicle type object here. Since the vehicle type's properties
    // are called quite often, it's much better to have this object readily
    // available, rather than take it by reference.
    ProblemData::VehicleType const vehicleType_;
    size_t const vehTypeIdx_;
    size_t const idx_;

    std::vector<Node *> nodes;  // Nodes in this route, including depots
    std::pair<double, double> centroid_;  // Center point of route's clients

    Node startDepot;  // Departure depot for this route
    Node endDepot;    // Return depot for this route

    std::vector<Distance> cumDist;  // Cumulative dist to each node (incl.)

    std::vector<LoadSegment> lsAt;      // Load data at each node
    std::vector<LoadSegment> lsAfter;   // LS of client -> depot (incl)
    std::vector<LoadSegment> lsBefore;  // LS of depot -> client (incl)

    std::vector<DurationSegment> dsAt;      // Duration data at each node
    std::vector<DurationSegment> dsAfter;   // DS of client -> depot (incl.)
    std::vector<DurationSegment> dsBefore;  // DS of depot -> client (incl.)

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

    // First client in the route if the route is non-empty. Else it is the
    // end depot. In either case the iterator is valid!
    [[nodiscard]] std::vector<Node *>::const_iterator begin() const;
    [[nodiscard]] std::vector<Node *>::iterator begin();

    // End depot. The iterator is valid!
    [[nodiscard]] std::vector<Node *>::const_iterator end() const;
    [[nodiscard]] std::vector<Node *>::iterator end();

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
     * Determines whether this route is time-feasible.
     *
     * @return true if the route has time warp, false otherwise.
     */
    [[nodiscard]] inline bool hasTimeWarp() const;

    /**
     * @return Total load on this route.
     */
    [[nodiscard]] inline Load load() const;

    /**
     * Load (as a consequence of pickup and deliveries) in excess of the
     * vehicle's capacity.
     */
    [[nodiscard]] inline Load excessLoad() const;

    /**
     * @return The load capacity of the vehicle servicing this route.
     */
    [[nodiscard]] inline Load capacity() const;

    /**
     * @return The location index of this route's depot.
     */
    [[nodiscard]] inline size_t depot() const;

    /**
     * @return The fixed cost of the vehicle servicing this route.
     */
    [[nodiscard]] inline Cost fixedVehicleCost() const;

    /**
     * @return Total distance travelled on this route.
     */
    [[nodiscard]] Distance distance() const;

    /**
     * @return The duration of this route.
     */
    [[nodiscard]] Duration duration() const;

    /**
     * @return The maximum duration of the vehicle servicing this route.
     */
    [[nodiscard]] inline Duration maxDuration() const;

    /**
     * @return Total time warp on this route.
     */
    [[nodiscard]] inline Duration timeWarp() const;

    /**
     * @return true if this route is empty, false otherwise.
     */
    [[nodiscard]] inline bool empty() const;

    /**
     * @return Number of clients in this route.
     */
    [[nodiscard]] inline size_t size() const;

    /**
     * Returns a proxy object that can be queried for data associated with
     * the node at idx.
     */
    [[nodiscard]] inline ProxyAt at(size_t idx) const;

    /**
     * Returns a proxy object that can be queried for data associated with
     * the segment starting at start.
     */
    [[nodiscard]] inline ProxyAfter after(size_t start) const;

    /**
     * Returns a proxy object that can be queried for data associated with
     * the segment ending at end.
     */
    [[nodiscard]] inline ProxyBefore before(size_t end) const;

    /**
     * Returns a proxy object that can be queried for data associated with
     * the segment between [start, end].
     */
    [[nodiscard]] inline ProxyBetween between(size_t start, size_t end) const;

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
     * Inserts the given node at index ``idx``. Assumes the given index is
     * valid.
     */
    void insert(size_t idx, Node *node);

    /**
     * Inserts the given node at the back of the route.
     */
    void push_back(Node *node);

    /**
     * Removes the node at ``idx`` from the route.
     */
    void remove(size_t idx);

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

Route *Route::Node::route() const { return route_; }

bool Route::Node::isDepot() const
{
    // We need to be in a route to be the depot. If we are, then we need to
    // be either the route's start or end depot.
    return route_ && (idx_ == 0 || idx_ == route_->size() + 1);
}

Route::ProxyAt::ProxyAt(Route const &route, size_t idx)
    : route(&route), idx(idx)
{
    assert(idx < route.nodes.size());
}

Route::ProxyAfter::ProxyAfter(Route const &route, size_t start)
    : route(&route), start(start)
{
    assert(start < route.nodes.size());
}

Route::ProxyBefore::ProxyBefore(Route const &route, size_t end)
    : route(&route), end(end)
{
    assert(end < route.nodes.size());
}

Route::ProxyBetween::ProxyBetween(Route const &route, size_t start, size_t end)
    : route(&route), start(start), end(end)
{
    assert(start <= end && end < route.nodes.size());
}

Route::ProxyAt::operator pyvrp::LoadSegment const &() const
{
    return route->lsAt[idx];
}

Route::ProxyAt::operator pyvrp::DurationSegment const &() const
{
    return route->dsAt[idx];
}

Route::ProxyAfter::operator pyvrp::LoadSegment const &() const
{
    return route->lsAfter[start];
}

Route::ProxyAfter::operator pyvrp::DurationSegment const &() const
{
    return route->dsAfter[start];
}

Route::ProxyBefore::operator pyvrp::LoadSegment const &() const
{
    return route->lsBefore[end];
}

Route::ProxyBefore::operator pyvrp::DurationSegment const &() const
{
    return route->dsBefore[end];
}

Route::ProxyBetween::operator Distance() const
{
    auto const startDist = route->cumDist[start];
    auto const endDist = route->cumDist[end];

    assert(startDist <= endDist);
    return endDist - startDist;
}

Route::ProxyBetween::operator LoadSegment() const
{
    auto ls = route->lsAt[start];

    for (size_t step = start; step != end; ++step)
        ls = LoadSegment::merge(ls, route->lsAt[step + 1]);

    return ls;
}

Route::ProxyBetween::operator DurationSegment() const
{
    auto ds = route->dsAt[start];

    for (size_t step = start; step != end; ++step)
        ds = DurationSegment::merge(
            route->data.durationMatrix(), ds, route->dsAt[step + 1]);

    return ds;
}

bool Route::isFeasible() const
{
    assert(!dirty);
    return !hasExcessLoad() && !hasTimeWarp();
}

bool Route::hasExcessLoad() const
{
    assert(!dirty);
    return load() > capacity();
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

Load Route::load() const
{
    assert(!dirty);
    return lsBefore.back().load();
}

Load Route::excessLoad() const
{
    assert(!dirty);
    return std::max<Load>(load() - capacity(), 0);
}

Load Route::capacity() const { return vehicleType_.capacity; }

size_t Route::depot() const { return vehicleType_.depot; }

Cost Route::fixedVehicleCost() const { return vehicleType_.fixedCost; }

Duration Route::maxDuration() const { return vehicleType_.maxDuration; }

Duration Route::timeWarp() const
{
    assert(!dirty);
    return dsBefore.back().timeWarp(maxDuration());
}

bool Route::empty() const { return size() == 0; }

size_t Route::size() const
{
    assert(nodes.size() >= 2);  // excl. depots
    return nodes.size() - 2;
}

Route::ProxyAt Route::at(size_t idx) const
{
    assert(!dirty);
    return ProxyAt(*this, idx);
}

Route::ProxyAfter Route::after(size_t start) const
{
    assert(!dirty);
    return ProxyAfter(*this, start);
}

Route::ProxyBefore Route::before(size_t end) const
{
    assert(!dirty);
    return ProxyBefore(*this, end);
}

Route::ProxyBetween Route::between(size_t start, size_t end) const
{
    assert(!dirty);
    return ProxyBetween(*this, start, end);
}
}  // namespace pyvrp::search

// Outputs a route into a given ostream in CVRPLib format
std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route);

#endif  // PYVRP_ROUTE_H
