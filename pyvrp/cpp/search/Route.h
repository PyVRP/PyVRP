#ifndef PYVRP_SEARCH_ROUTE_H
#define PYVRP_SEARCH_ROUTE_H

#include "DistanceSegment.h"
#include "DurationSegment.h"
#include "LoadSegment.h"
#include "ProblemData.h"

#include <algorithm>
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
public:
    /**
     * A simple class that tracks a proposed route structure. This new structure
     * can be efficiently evaluated by calling appropriate member functions for
     * concatenation schemes detailing the newly proposed route's statistics.
     */
    template <typename... Segments> class Proposal
    {
        std::tuple<Segments...> segments_;

    public:
        Proposal(Segments &&...segments);

        /**
         * The proposal's route. This is the route associated with the first
         * segment, and determines i.a. the vehicle type and route profile used
         * in the proposal.
         */
        Route const *route() const;

        DistanceSegment distanceSegment() const;
        DurationSegment durationSegment() const;
        LoadSegment loadSegment(size_t dimension) const;
    };

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

        /**
         * Assigns the node to the given route, at the given index.
         */
        void assign(Route *route, size_t idx);

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
        Route const &route_;
        size_t const start;

    public:
        inline Route const *route() const;

        inline size_t first() const;  // client at start
        inline size_t last() const;   // end depot

        inline SegmentAfter(Route const &route, size_t start);
        inline DistanceSegment distance(size_t profile) const;
        inline DurationSegment duration(size_t profile) const;
        inline LoadSegment const &load(size_t dimension) const;
    };

    /**
     * Class storing data related to the route segment starting at the depot,
     * and ending at ``end`` (inclusive).
     */
    class SegmentBefore
    {
        Route const &route_;
        size_t const end;

    public:
        inline Route const *route() const;

        inline size_t first() const;  // start depot
        inline size_t last() const;   // client at end

        inline SegmentBefore(Route const &route, size_t end);
        inline DistanceSegment distance(size_t profile) const;
        inline DurationSegment duration(size_t profile) const;
        inline LoadSegment const &load(size_t dimension) const;
    };

    /**
     * Class storing data related to the route segment starting at ``start``,
     * and ending at ``end`` (inclusive).
     */
    class SegmentBetween
    {
        Route const &route_;
        size_t const start;
        size_t const end;

    public:
        inline Route const *route() const;

        inline size_t first() const;  // client at start
        inline size_t last() const;   // client at end

        inline SegmentBetween(Route const &route, size_t start, size_t end);
        inline DistanceSegment distance(size_t profile) const;
        inline DurationSegment duration(size_t profile) const;
        inline LoadSegment load(size_t dimension) const;
    };

    ProblemData const &data;

    ProblemData::VehicleType const &vehicleType_;
    size_t const idx_;

    Node startDepot_;  // Departure depot for this route
    Node endDepot_;    // Return depot for this route

    std::vector<Node *> nodes;   // Nodes in this route, including depots
    std::vector<size_t> visits;  // Locations in this route, incl. depots
    std::pair<double, double> centroid_;  // Center point of route's clients

    std::vector<Distance> cumDist;  // Dist of depot -> client (incl.)

    // Load data, for each load dimension. These vectors form matrices, where
    // the rows index the load dimension, and the columns the nodes.
    std::vector<LoadSegments> loadAt;      // Load data at each node
    std::vector<LoadSegments> loadAfter;   // Load of client -> depot (incl)
    std::vector<LoadSegments> loadBefore;  // Load of depot -> client (incl)

    std::vector<Load> load_;        // Route loads (for each dimension)
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
     * Total loads on this route.
     */
    [[nodiscard]] inline std::vector<Load> const &load() const;

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
     * @return Number of clients in this route.
     */
    [[nodiscard]] inline size_t size() const;

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
     * Inserts the given node before index ``idx``. Assumes the given index is
     * valid.
     */
    void insert(size_t idx, Node *node);

    /**
     * Inserts the given range of nodes before ``idx``. Assumes the given index
     * is valid.
     */
    template <class InputIt>
    void insert(size_t idx, InputIt first, InputIt last);

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
    return route_
           && (this == &route_->startDepot_ || this == &route_->endDepot_);
}

Route::SegmentAfter::SegmentAfter(Route const &route, size_t start)
    : route_(route), start(start)
{
    assert(start < route.nodes.size());
}

Route::SegmentBefore::SegmentBefore(Route const &route, size_t end)
    : route_(route), end(end)
{
    assert(end < route.nodes.size());
}

Route::SegmentBetween::SegmentBetween(Route const &route,
                                      size_t start,
                                      size_t end)
    : route_(route), start(start), end(end)
{
    assert(start <= end && end < route.nodes.size());
}

DistanceSegment
Route::SegmentAfter::distance([[maybe_unused]] size_t profile) const
{
    assert(profile == route_.profile());
    return {route_.cumDist.back() - route_.cumDist[start]};
}

DurationSegment
Route::SegmentAfter::duration([[maybe_unused]] size_t profile) const
{
    assert(profile == route_.profile());
    return route_.durAfter[start];
}

LoadSegment const &Route::SegmentAfter::load(size_t dimension) const
{
    return route_.loadAfter[dimension][start];
}

DistanceSegment
Route::SegmentBefore::distance([[maybe_unused]] size_t profile) const
{
    assert(profile == route_.profile());
    return {route_.cumDist[end]};
}

DurationSegment
Route::SegmentBefore::duration([[maybe_unused]] size_t profile) const
{
    assert(profile == route_.profile());
    return route_.durBefore[end];
}

LoadSegment const &Route::SegmentBefore::load(size_t dimension) const
{
    return route_.loadBefore[dimension][end];
}

Route const *Route::SegmentBefore::route() const { return &route_; }
size_t Route::SegmentBefore::first() const { return route_.visits.front(); }
size_t Route::SegmentBefore::last() const { return route_.visits[end]; }

Route const *Route::SegmentAfter::route() const { return &route_; }
size_t Route::SegmentAfter::first() const { return route_.visits[start]; }
size_t Route::SegmentAfter::last() const { return route_.visits.back(); }

Route const *Route::SegmentBetween::route() const { return &route_; }
size_t Route::SegmentBetween::first() const { return route_.visits[start]; }
size_t Route::SegmentBetween::last() const { return route_.visits[end]; }

DistanceSegment Route::SegmentBetween::distance(size_t profile) const
{
    if (profile != route_.profile())  // then we have to compute the distance
    {                                 // segment from scratch.
        auto const &mat = route_.data.distanceMatrix(profile);
        DistanceSegment distSegment = {0};

        for (size_t step = start; step != end; ++step)
        {
            auto const from = route_.visits[step];
            auto const to = route_.visits[step + 1];
            distSegment
                = DistanceSegment::merge(mat(from, to), distSegment, {0});
        }

        return distSegment;
    }

    auto const startDist = route_.cumDist[start];
    auto const endDist = route_.cumDist[end];

    assert(startDist <= endDist);
    return {endDist - startDist};
}

DurationSegment Route::SegmentBetween::duration(size_t profile) const
{
    auto const &mat = route_.data.durationMatrix(profile);
    auto durSegment = route_.durAt[start];

    for (size_t step = start; step != end; ++step)
    {
        auto const from = route_.visits[step];
        auto const to = route_.visits[step + 1];
        auto const &durAt = route_.durAt[step + 1];
        durSegment = DurationSegment::merge(mat(from, to), durSegment, durAt);
    }

    return durSegment;
}

LoadSegment Route::SegmentBetween::load(size_t dimension) const
{
    auto const &loads = route_.loadAt[dimension];

    auto loadSegment = loads[start];
    for (size_t step = start; step != end; ++step)
        loadSegment = LoadSegment::merge(loadSegment, loads[step + 1]);

    return loadSegment;
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

std::vector<Load> const &Route::load() const
{
    assert(!dirty);
    return load_;
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

bool Route::empty() const { return size() == 0; }

size_t Route::size() const
{
    assert(nodes.size() >= 2);  // excl. depots
    return nodes.size() - 2;
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

template <typename... Segments>
Route::Proposal<Segments...>::Proposal(Segments &&...segments)
    : segments_(std::forward<Segments>(segments)...)
{
    static_assert(sizeof...(Segments) > 0, "Proposal cannot be empty.");

    [[maybe_unused]] auto &&first = std::get<0>(segments_);
    [[maybe_unused]] auto &&last = std::get<sizeof...(Segments) - 1>(segments_);
    assert(first.route() == last.route());  // must start and end at same route
}

template <typename... Segments>
Route const *Route::Proposal<Segments...>::route() const
{
    return std::get<0>(segments_).route();
}

template <typename... Segments>
DistanceSegment Route::Proposal<Segments...>::distanceSegment() const
{
    auto const &data = route()->data;
    auto const profile = route()->profile();
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

    return std::apply(fn, segments_);
}

template <typename... Segments>
DurationSegment Route::Proposal<Segments...>::durationSegment() const
{
    auto const &data = route()->data;
    auto const profile = route()->profile();
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

    return std::apply(fn, segments_);
}

template <typename... Segments>
LoadSegment Route::Proposal<Segments...>::loadSegment(size_t dimension) const
{
    auto const fn = [dimension](auto &&...args)
    {
        LoadSegment segment;

        auto const merge = [&](auto const &self, auto &&other, auto &&...args)
        {
            segment = LoadSegment::merge(segment, other);
            if constexpr (sizeof...(args) != 0)
                self(self, std::forward<decltype(args)>(args)...);
        };

        merge(merge, args.load(dimension)...);
        return segment;
    };

    return std::apply(fn, segments_);
}

template <class InputIt>
void Route::insert(size_t idx, InputIt first, InputIt last)
{
    assert(0 < idx && idx < nodes.size());
    nodes.insert(nodes.begin() + idx, first, last);

    for (size_t after = idx; after != nodes.size(); ++after)
        nodes[after]->assign(this, after);

#ifndef NDEBUG
    dirty = true;
#endif
}
}  // namespace pyvrp::search

// Outputs a route into a given ostream in CVRPLib format
std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route);

#endif  // PYVRP_SEARCH_ROUTE_H
