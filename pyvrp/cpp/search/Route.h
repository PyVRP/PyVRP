#ifndef PYVRP_ROUTE_H
#define PYVRP_ROUTE_H

#include "ProblemData.h"
#include "TimeWindowSegment.h"

#include <cassert>
#include <iosfwd>

namespace pyvrp::search
{
class Route
{
public:
    class Node
    {
    public:  // TODO make fields private
        // TODO rename client to location/loc
        size_t client;           // Location represented by this node
        size_t idx = 0;          // Position in the route
        Route *route = nullptr;  // Indicates membership of a route, if any.

        // TODO can these data fields be moved to Route?
        TimeWindowSegment tw;        // TWS for individual node (client)
        TimeWindowSegment twBefore;  // TWS for (0...client) including self
        TimeWindowSegment twAfter;   // TWS for (client...0) including self

        Node(size_t client);

        [[nodiscard]] inline bool isDepot() const;
    };

private:
    ProblemData const &data;
    size_t const vehicleType_;

    std::vector<Node *> nodes;      // Nodes in this route, excl. depot
    std::vector<Distance> cumDist;  // Cumulative dist along route (incl.)
    std::vector<Load> cumLoad;      // Cumulative load along route (incl.)

    std::pair<double, double> centroid;  // Center point of route's clients

    Node startDepot;  // Departure depot for this route
    Node endDepot;    // Return depot for this route

public:                // TODO make fields private
    size_t const idx;  // Route index

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
    [[nodiscard]] inline TimeWindowSegment twBetween(size_t start,
                                                     size_t end) const;

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

    Route(ProblemData const &data, size_t const idx, size_t const vehType);
};

/**
 * Convenience method accessing the node directly before the argument.
 */
inline Route::Node *p(Route::Node *node)
{
    auto &route = *node->route;
    return route[node->idx - 1];
}

/**
 * Convenience method accessing the node directly after the argument.
 */
inline Route::Node *n(Route::Node *node)
{
    auto &route = *node->route;
    return route[node->idx + 1];
}

bool Route::Node::isDepot() const
{
    // We need to be in a route to be the depot. If we are, then we need to
    // be either the route's start or end depot.
    return route && (this == &route->startDepot || this == &route->endDepot);
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

Duration Route::timeWarp() const { return endDepot.twBefore.totalTimeWarp(); }

Load Route::capacity() const { return data.vehicleType(vehicleType_).capacity; }

bool Route::empty() const { return size() == 0; }

size_t Route::size() const
{
    assert(nodes.size() >= 2);
    return nodes.size() - 2;
}

TimeWindowSegment Route::twBetween(size_t start, size_t end) const
{
    assert(0 < start && start <= end && end < nodes.size());

    auto *node = nodes[start];
    auto tws = node->tw;

    for (size_t step = start; step != end; ++step)
    {
        node = n(node);
        tws = TimeWindowSegment::merge(data.durationMatrix(), tws, node->tw);
    }

    return tws;
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

    auto const atStart = data.client(nodes[start]->client).demand;
    auto const startLoad = cumLoad[start];
    auto const endLoad = cumLoad[end];

    assert(startLoad <= endLoad);
    return endLoad - startLoad + atStart;
}
}  // namespace pyvrp::search

// Outputs a route into a given ostream in CVRPLib format
std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route);

#endif  // PYVRP_ROUTE_H
