#ifndef HGS_VRPTW_ROUTE_H
#define HGS_VRPTW_ROUTE_H

#include "Node.h"
#include "TimeWindowSegment.h"

#include <array>
#include <bit>
#include <cassert>
#include <iosfwd>

class Route
{
    std::vector<Node *> nodes;  // List of nodes (in order) in this solution.

    // Populates the nodes vector.
    void setupNodes();

    // Sets the route center angle.
    void setupAngle();

    // Sets forward node time windows.
    void setupRouteTimeWindows();

public:  // TODO make fields private
    ProblemData const *data;

    int idx;             // Route index
    Node *depot;         // Pointer to the associated depot
    double angleCenter;  // Angle of the barycenter of the route

    /**
     * Returns the client or depot node at the given position.
     */
    [[nodiscard]] Node *operator[](size_t position) const;

    /**
     * Tests if this route is feasible.
     */
    [[nodiscard]] bool isFeasible() const;

    /**
     * Determines whether this route is load-feasible.
     */
    [[nodiscard]] bool hasExcessCapacity() const;

    /**
     * Determines whether this route is time-feasible.
     */
    [[nodiscard]] bool hasTimeWarp() const;

    /**
     * Returns total load on this route.
     */
    [[nodiscard]] int load() const;

    /**
     * Returns total time warp on this route.
     */
    [[nodiscard]] int timeWarp() const;

    [[nodiscard]] bool empty() const;

    [[nodiscard]] size_t size() const;

    /**
     * Calculates time window data for segment [start, end].
     */
    [[nodiscard]] inline TimeWindowSegment twBetween(size_t start,
                                                     size_t end) const;

    /**
     * Calculates the distance for segment [start, end].
     */
    [[nodiscard]] inline int distBetween(size_t start, size_t end) const;

    /**
     * Calculates the load for segment [start, end].
     */
    [[nodiscard]] inline int loadBetween(size_t start, size_t end) const;

    /**
     * Updates this route. To be called after swapping nodes/changing the
     * solution.
     */
    void update();
};

TimeWindowSegment Route::twBetween(size_t start, size_t end) const
{
    assert(start <= end);

    auto tws = nodes[start - 1]->tw;

    for (size_t step = start; step != end; ++step)
        tws = TimeWindowSegment::merge(tws, nodes[step]->tw);

    return tws;
}

int Route::distBetween(size_t start, size_t end) const
{
    assert(start <= end && end <= nodes.size());

    auto const startDist = start == 0 ? 0 : nodes[start - 1]->cumulatedDistance;
    auto const endDist = nodes[end - 1]->cumulatedDistance;

    assert(startDist <= endDist);

    return endDist - startDist;
}

int Route::loadBetween(size_t start, size_t end) const
{
    assert(start <= end && end <= nodes.size());

    auto const *startNode = start == 0 ? depot : nodes[start - 1];
    auto const atStart = data->client(startNode->client).demand;
    auto const startLoad = startNode->cumulatedLoad;
    auto const endLoad = nodes[end - 1]->cumulatedLoad;

    assert(startLoad <= endLoad);

    return endLoad - startLoad + atStart;
}

// Outputs a route into a given ostream in CVRPLib format
std::ostream &operator<<(std::ostream &out, Route const &route);

#endif  // HGS_VRPTW_ROUTE_H
