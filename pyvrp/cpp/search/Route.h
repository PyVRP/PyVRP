#ifndef PYVRP_ROUTE_H
#define PYVRP_ROUTE_H

#include "CircleSector.h"
#include "Node.h"
#include "ProblemData.h"
#include "TimeWindowSegment.h"

#include <array>
#include <bit>
#include <cassert>
#include <iosfwd>

class Route
{
    ProblemData const &data;

    std::vector<Node *> nodes;  // List of nodes (in order) in this solution.
    CircleSector sector;        // Circle sector of the route's clients

    Load weight_;            // Current route weight load.
    Load volume_;            // Current route volume load.
    Salvage salvage_;        // Current route salvage demand.
    bool isWeightFeasible_;  // Whether current weight load is feasible.
    bool isVolumeFeasible_;  // Whether current volume load is feasible.
    bool isSalvageCapacityFeasible_;  // Whether current salvage demand is salvage capacity feasible.
    bool isSalvageSequenceFeasible_;  // Whether current route sequence salvage sequence feasible.

    Duration timeWarp_;        // Current route time warp.
    bool isTimeWarpFeasible_;  // Whether current time warp is feasible.

    // Populates the nodes vector.
    void setupNodes();

    // Sets the sector data.
    void setupSector();

    // Sets forward node time windows.
    void setupRouteTimeWindows();

public:           // TODO make fields private
    int idx;      // Route index
    Node *depot;  // Pointer to the associated depot

    /**
     * @return The client or depot node at the given position.
     */
    [[nodiscard]] inline Node *operator[](size_t position) const;

    /**
     * Tests if this route is feasible.
     *
     * @return true if the route is feasible, false otherwise.
     */
    [[nodiscard]] inline bool isFeasible() const;

    /**
     * Determines whether this route is weight load-feasible.
     *
     * @return true if the route exceeds the vehicle weight capacity, false otherwise.
     */
    [[nodiscard]] inline bool hasExcessWeight() const;

    /**
     * Determines whether this route is volume load-feasible.
     *
     * @return true if the route exceeds the vehicle volume capacity, false otherwise.
     */
    [[nodiscard]] inline bool hasExcessVolume() const;

    /**
     * Determines whether this route is salvage-feasible.
     *
     * @return true if the route exceeds the salvage capacity, false otherwise.
     */
    [[nodiscard]] inline bool hasExcessSalvage() const;

    /**
     * Determines whether this route is salvage-feasible.
     *
     * @return true if the route violates the salvage sequence constraint
     */
    [[nodiscard]] inline bool hasSalvageBeforeDelivery() const;

    /**
     * Determines whether this route is time-feasible.
     *
     * @return true if the route has time warp, false otherwise.
     */
    [[nodiscard]] inline bool hasTimeWarp() const;

    /**
     * @return Total weight load on this route.
     */
    [[nodiscard]] inline Load weight() const;

    /**
     * @return Total volume load on this route.
     */
    [[nodiscard]] inline Load volume() const;

    /**
     * @return Total salvage demand on this route.
     */
    [[nodiscard]] inline Salvage salvage() const;

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
    [[nodiscard]] inline Load weightBetween(size_t start, size_t end) const;

    /**
     * Calculates the load for segment [start, end].
     */
    [[nodiscard]] inline Load volumeBetween(size_t start, size_t end) const;

    /**
     * Calculates the demand for segment [start, end].
     */
    [[nodiscard]] inline Salvage salvageBetween(size_t start, size_t end) const;

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

    Route(ProblemData const &data);
};

bool Route::isFeasible() const { return !hasExcessWeight() && !hasExcessVolume() && !hasExcessSalvage() && !hasSalvageBeforeDelivery() && !hasTimeWarp(); }

bool Route::hasExcessWeight() const { return !isWeightFeasible_; }

bool Route::hasExcessVolume() const { return !isVolumeFeasible_; }

bool Route::hasExcessSalvage() const { return !isSalvageCapacityFeasible_; }

bool Route::hasSalvageBeforeDelivery() const { return !isSalvageSequenceFeasible_; }

bool Route::hasTimeWarp() const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return false;
#else
    return !isTimeWarpFeasible_;
#endif
}

Node *Route::operator[](size_t position) const
{
    assert(position > 0);
    return nodes[position - 1];
}

Load Route::weight() const { return weight_; }

Load Route::volume() const { return volume_; }

Salvage Route::salvage() const { return salvage_; }

Duration Route::timeWarp() const { return timeWarp_; }

bool Route::empty() const { return size() == 0; }

size_t Route::size() const
{
    return nodes.size() - 1;  // exclude end depot
}

TimeWindowSegment Route::twBetween(size_t start, size_t end) const
{
    assert(0 < start && start <= end && end <= nodes.size());

    auto tws = nodes[start - 1]->tw;

    for (size_t step = start; step != end; ++step)
        tws = TimeWindowSegment::merge(
            data.durationMatrix(), tws, nodes[step]->tw);

    return tws;
}

Distance Route::distBetween(size_t start, size_t end) const
{
    assert(start <= end && end <= nodes.size());

    auto const startDist = start == 0 ? 0 : nodes[start - 1]->cumulatedDistance;
    auto const endDist = nodes[end - 1]->cumulatedDistance;

    assert(startDist <= endDist);

    return endDist - startDist;
}

Load Route::weightBetween(size_t start, size_t end) const
{
    assert(start <= end && end <= nodes.size());

    auto const *startNode = start == 0 ? depot : nodes[start - 1];
    auto const atStart = data.client(startNode->client).demandWeight;
    auto const startWeight = startNode->cumulatedWeight;
    auto const endWeight = nodes[end - 1]->cumulatedWeight;

    assert(startWeight <= endWeight);

    return endWeight - startWeight + atStart;
}

Load Route::volumeBetween(size_t start, size_t end) const
{   
    assert(start <= end && end <= nodes.size());
    
    auto const *startNode = start == 0 ? depot : nodes[start - 1];
    auto const atStart = data.client(startNode->client).demandVolume;
    auto const startVolume = startNode->cumulatedVolume;
    auto const endVolume = nodes[end - 1]->cumulatedVolume;
    
    assert(startVolume <= endVolume);
     
    return endVolume - startVolume + atStart;
}

Salvage Route::salvageBetween(size_t start, size_t end) const
{    
    assert(start <= end && end <= nodes.size());
     
    auto const *startNode = start == 0 ? depot : nodes[start - 1];
    auto const atStart = data.client(startNode->client).demandSalvage;
    auto const startSalvage = startNode->cumulatedSalvage;
    auto const endSalvage = nodes[end - 1]->cumulatedSalvage;
     
    assert(startSalvage <= endSalvage);
     
    return endSalvage - startSalvage + atStart;
}

// Outputs a route into a given ostream in CVRPLib format
std::ostream &operator<<(std::ostream &out, Route const &route);

#endif  // PYVRP_ROUTE_H
