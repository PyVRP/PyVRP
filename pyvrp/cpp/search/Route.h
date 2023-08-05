#ifndef PYVRP_ROUTE_H
#define PYVRP_ROUTE_H

#include "ProblemData.h"
#include "TimeWindowSegment.h"

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
 *    statitics like time window data, or load and distance attributes. To make
 *    that happen, ``Route::update()`` must be called!
 */
class Route
{
public:
    /**
     * Light wrapper class around a client or depot location. This class tracks
     * the route it is in, and the position and role it currently has in that
     * route.
     */
    class Node
    {
        friend class Route;

        size_t loc_;             // Location represented by this node
        size_t idx_;             // Position in the route
        Route *route_;           // Indicates membership of a route, if any
        TimeWindowSegment tws_;  // This location's time window data

    public:
        Node(size_t loc, ProblemData::Client const &client);

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
         * Returns this location's time window segment data. This is the time
         * window data of just this location.
         */
        [[nodiscard]] inline TimeWindowSegment const &tws() const;

        /**
         * Returns whether this node is a depot. A node can only be a depot if
         * it is in a route.
         */
        [[nodiscard]] inline bool isDepot() const;
    };

private:
    ProblemData const &data;
    size_t const vehicleType_;
    size_t const idx_;

    std::vector<Node *> nodes;      // Nodes in this route, including depots
    std::vector<Distance> cumDist;  // Cumulative dist along route (incl.)
    std::vector<Load> cumLoad;      // Cumulative load along route (incl.)

    std::vector<TimeWindowSegment> twsBefore_;  // TWS of depot -> client
    std::vector<TimeWindowSegment> twsAfter_;   // TWS of client -> depot

    std::pair<double, double> centroid;  // Center point of route's clients

    Node startDepot;  // Departure depot for this route
    Node endDepot;    // Return depot for this route

public:
    /**
     * Route index.
     */
    [[nodiscard]] inline size_t idx() const;

    /**
     * @return The client or depot node at the given ``idx``.
     */
    [[nodiscard]] inline Node *operator[](size_t idx);

    [[nodiscard]] inline std::vector<Node *>::const_iterator begin() const;
    [[nodiscard]] inline std::vector<Node *>::const_iterator end() const;

    [[nodiscard]] inline std::vector<Node *>::iterator begin();
    [[nodiscard]] inline std::vector<Node *>::iterator end();

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
     * @return Total time warp on this route.
     */
    [[nodiscard]] inline Duration timeWarp() const;

    /**
     * @return The load capacity of this route.
     */
    [[nodiscard]] inline Load capacity() const;

    /**
     * @return true if this route is empty, false otherwise.
     */
    [[nodiscard]] inline bool empty() const;

    /**
     * @return Number of clients in this route.
     */
    [[nodiscard]] inline size_t size() const;

    /**
     * Calculates time window data for segment [start, end].
     */
    [[nodiscard]] inline TimeWindowSegment twsBetween(size_t start,
                                                      size_t end) const;

    /**
     * Returns time window data for segment [start, 0].
     */
    [[nodiscard]] inline TimeWindowSegment twsAfter(size_t start) const;

    /**
     * Returns time window data for segment [0, end].
     */
    [[nodiscard]] inline TimeWindowSegment twsBefore(size_t end) const;

    /**
     * Calculates the distance for segment [start, end].
     */
    [[nodiscard]] inline Distance distBetween(size_t start, size_t end) const;

    /**
     * Calculates the load for segment [start, end].
     */
    [[nodiscard]] inline Load loadBetween(size_t start, size_t end) const;

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

[[nodiscard]] inline TimeWindowSegment const &Route::Node::tws() const
{
    return tws_;
}

bool Route::Node::isDepot() const
{
    // We need to be in a route to be the depot. If we are, then we need to
    // be either the route's start or end depot.
    return route_ && (idx_ == 0 || idx_ == route_->size() + 1);
}

bool Route::isFeasible() const { return !hasExcessLoad() && !hasTimeWarp(); }

bool Route::hasExcessLoad() const { return load() > capacity(); }

bool Route::hasTimeWarp() const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return false;
#else
    return timeWarp() > 0;
#endif
}

size_t Route::idx() const { return idx_; }

Route::Node *Route::operator[](size_t idx)
{
    assert(idx < nodes.size());
    return nodes[idx];
}

std::vector<Route::Node *>::const_iterator Route::begin() const
{
    return nodes.begin() + 1;
}
std::vector<Route::Node *>::const_iterator Route::end() const
{
    return nodes.end() - 1;
}

std::vector<Route::Node *>::iterator Route::begin()
{
    return nodes.begin() + 1;
}
std::vector<Route::Node *>::iterator Route::end() { return nodes.end() - 1; }

Load Route::load() const { return cumLoad.back(); }

Duration Route::timeWarp() const { return twsBefore_.back().totalTimeWarp(); }

Load Route::capacity() const { return data.vehicleType(vehicleType_).capacity; }

bool Route::empty() const { return size() == 0; }

size_t Route::size() const
{
    assert(nodes.size() >= 2);  // excl. depots
    return nodes.size() - 2;
}

TimeWindowSegment Route::twsBetween(size_t start, size_t end) const
{
    using TWS = TimeWindowSegment;
    assert(start <= end && end < nodes.size());

    if (start == end)  // shortcut in case we want the node time window data
        return nodes[start]->tws();

    auto tws = nodes[start]->tws();

    for (size_t step = start + 1; step != end; ++step)
        tws = TWS::merge(data.durationMatrix(), tws, nodes[step]->tws());

    return tws;
}

TimeWindowSegment Route::twsAfter(size_t start) const
{
    assert(start < nodes.size());
    return twsAfter_[start];
}

TimeWindowSegment Route::twsBefore(size_t end) const
{
    assert(end < nodes.size());
    return twsBefore_[end];
}

Distance Route::distBetween(size_t start, size_t end) const
{
    assert(start <= end && end < nodes.size());

    auto const startDist = cumDist[start];
    auto const endDist = cumDist[end];

    assert(startDist <= endDist);
    return endDist - startDist;
}

Load Route::loadBetween(size_t start, size_t end) const
{
    assert(start <= end && end < nodes.size());

    auto const atStart = data.client(nodes[start]->client()).demand;
    auto const startLoad = cumLoad[start];
    auto const endLoad = cumLoad[end];

    assert(startLoad <= endLoad);
    return endLoad - startLoad + atStart;
}
}  // namespace pyvrp::search

// Outputs a route into a given ostream in CVRPLib format
std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route);

#endif  // PYVRP_ROUTE_H
