#ifndef PYVRP_ROUTE_H
#define PYVRP_ROUTE_H

#include "CircleSector.h"
#include "ProblemData.h"
#include "TimeWindowSegment.h"

#include <cassert>
#include <iosfwd>

namespace pyvrp::search
{
class Route
{
public:
    struct Node
    {
        // TODO rename client to location/loc
        size_t client;           // Location represented by this node
        size_t position = 0;     // Position in the route
        Route *route = nullptr;  // Indicates membership of a route, if any.
        Node *prev = nullptr;    // Predecessor in route
        Node *next = nullptr;    // Successor in route

        // TODO can these data fields be moved to Route?
        Load cumulatedLoad = 0;              // Load depot -> client (incl)
        Distance cumulatedDistance = 0;      // Dist depot - client (incl)
        Distance deltaReversalDistance = 0;  // Delta of depot - client reversed

        TimeWindowSegment tw;        // TWS for individual node (client)
        TimeWindowSegment twBefore;  // TWS for (0...client) including self
        TimeWindowSegment twAfter;   // TWS for (client...0) including self

        Node(size_t client);

        [[nodiscard]] inline bool isDepot() const;

        /**
         * Inserts this node after the other and updates the relevant links.
         */
        void insertAfter(Node *other);

        /**
         * Swaps this node with the other and updates the relevant links.
         */
        void swapWith(Node *other);

        /**
         * Removes this node and updates the relevant links.
         */
        void remove();
    };

private:
    ProblemData const &data;
    size_t const vehicleType_;

    std::vector<Node *> nodes;  // List of nodes in this route, excl. depot
    CircleSector sector;        // Circle sector of the route's clients

    Load load_;          // Current route load.
    Duration timeWarp_;  // Current route time warp.

    // Sets the sector data.
    void setupSector();

    // Sets forward node time windows.
    void setupRouteTimeWindows();

public:                // TODO make fields private
    size_t const idx;  // Route index
    Node startDepot;   // Departure depot for this route
    Node endDepot;     // Return depot for this route

    /**
     * @return The client or depot node at the given position.
     */
    [[nodiscard]] inline Node *operator[](size_t position) const;

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
     * Tests if this route overlaps with the other route, that is, whether
     * their circle sectors overlap with a given tolerance.
     */
    [[nodiscard]] bool overlapsWith(Route const &other,
                                    int const tolerance) const;

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
inline Route::Node *p(Route::Node *node) { return node->prev; }

/**
 * Convenience method accessing the node directly after the argument.
 */
inline Route::Node *n(Route::Node *node) { return node->next; }

bool Route::Node::isDepot() const
{
    // We need to be in a route to be the depot. If we are, then we need to
    // be either the route's start or end depot.
    return route && (this == &route->startDepot || this == &route->endDepot);
}

bool Route::isFeasible() const { return !hasExcessLoad() && !hasTimeWarp(); }

bool Route::hasExcessLoad() const { return load_ > capacity(); }

bool Route::hasTimeWarp() const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return false;
#else
    return timeWarp_ > 0;
#endif
}

Route::Node *Route::operator[](size_t position) const
{
    assert(position > 0);
    return nodes[position - 1];
}

std::vector<Route::Node *>::const_iterator Route::begin() const
{
    return nodes.begin();
}
std::vector<Route::Node *>::const_iterator Route::end() const
{
    return nodes.end();
}

std::vector<Route::Node *>::iterator Route::begin() { return nodes.begin(); }
std::vector<Route::Node *>::iterator Route::end() { return nodes.end(); }

Load Route::load() const { return load_; }

Duration Route::timeWarp() const { return timeWarp_; }

Load Route::capacity() const { return data.vehicleType(vehicleType_).capacity; }

bool Route::empty() const { return size() == 0; }

size_t Route::size() const { return nodes.size(); }

TimeWindowSegment Route::twBetween(size_t start, size_t end) const
{
    assert(0 < start && start <= end && end <= nodes.size() + 1);

    auto tws = nodes[start - 1]->tw;
    auto *node = nodes[start - 1];

    for (size_t step = start; step != end; ++step)
    {
        node = n(node);
        tws = TimeWindowSegment::merge(data.durationMatrix(), tws, node->tw);
    }

    return tws;
}

Distance Route::distBetween(size_t start, size_t end) const
{
    assert(start <= end && end <= nodes.size() + 1);

    auto const startDist = start == 0 ? 0 : nodes[start - 1]->cumulatedDistance;
    auto const endDist = end == nodes.size() + 1
                             ? endDepot.cumulatedDistance
                             : nodes[end - 1]->cumulatedDistance;

    assert(startDist <= endDist);

    return endDist - startDist;
}

Load Route::loadBetween(size_t start, size_t end) const
{
    assert(start <= end && end <= nodes.size() + 1);

    auto const *startNode = start == 0 ? &startDepot : nodes[start - 1];
    auto const atStart = data.client(startNode->client).demand;
    auto const startLoad = startNode->cumulatedLoad;
    auto const endLoad = end == nodes.size() + 1
                             ? endDepot.cumulatedLoad
                             : nodes[end - 1]->cumulatedLoad;

    assert(startLoad <= endLoad);

    return endLoad - startLoad + atStart;
}
}  // namespace pyvrp::search

// Outputs a route into a given ostream in CVRPLib format
std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route);

#endif  // PYVRP_ROUTE_H
