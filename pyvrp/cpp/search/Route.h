#ifndef PYVRP_SEARCH_ROUTE_H
#define PYVRP_SEARCH_ROUTE_H

#include "DurationSegment.h"
#include "LoadSegment.h"
#include "ProblemData.h"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <iosfwd>
#include <utility>

namespace pyvrp::search
{
// This defines the minimal interface required for a segment of visits.
template <typename T>
concept Segment = requires(T arg, size_t profile, size_t dimension) {
    { arg.route() };
    { arg.first() } -> std::same_as<size_t>;
    { arg.last() } -> std::same_as<size_t>;
    { arg.distance(profile) } -> std::convertible_to<Distance>;
    { arg.duration(profile) } -> std::convertible_to<DurationSegment>;
    { arg.load(dimension) } -> std::convertible_to<LoadSegment>;
};

namespace detail
{
template <class Tuple, std::size_t... Indices>
auto constexpr reverse_impl(Tuple &&tuple, std::index_sequence<Indices...>)
{
    return std::make_tuple(std::get<sizeof...(Indices) - 1 - Indices>(
        std::forward<Tuple>(tuple))...);
}

template <class Tuple> auto constexpr reverse(Tuple &&tuple)
{
    auto constexpr size = std::tuple_size_v<std::remove_reference_t<Tuple>>;
    auto constexpr indices = std::make_index_sequence<size>{};
    return reverse_impl(tuple, indices);
}
}  // namespace detail

/**
 * This ``Route`` class supports fast delta cost computations and in-place
 * modification. It can be used to implement move evaluations.
 *
 * A ``Route`` object tracks a full route, including the depots. The clients
 * and depots on the route can be accessed using ``Route::operator[]`` on a
 * ``route`` object.
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
     * can be efficiently evaluated by calling appropriate member functions,
     * detailing the newly proposed route's statistics.
     */
    template <Segment... Segments> class Proposal
    {
        std::tuple<Segments...> segments_;

    public:
        Proposal(Segments &&...segments);

        /**
         * The proposal's route. This is the route associated with the first
         * and last segments, and determines the vehicle type and route profile
         * used when evaluating the proposal.
         */
        Route const *route() const;

        /**
         * Returns the travel distance of the proposed route.
         */
        Distance distance() const;

        /**
         * Returns a pair of (duration, time warp) attributes of the proposed
         * route.
         */
        std::pair<Duration, Duration> duration() const;

        /**
         * Returns the excess load of the proposed route.
         */
        Load excessLoad(size_t dimension) const;
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
        size_t trip_;   // Trip index.
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
         * Returns this node's assigned trip number.  This value is ``0`` when
         * the node is *not* in a route.
         */
        [[nodiscard]] inline size_t trip() const;

        /**
         * Returns the route this node is currently in. If the node is not in
         * a route, this returns ``None`` (C++: ``nullptr``).
         */
        [[nodiscard]] inline Route *route() const;

        /**
         * Returns whether this node is a depot.
         */
        [[nodiscard]] inline bool isDepot() const;

        /**
         * Returns whether this node is a start depot.
         */
        [[nodiscard]] inline bool isStartDepot() const;

        /**
         * Returns whether this node is an end depot.
         */
        [[nodiscard]] inline bool isEndDepot() const;

        /**
         * Returns whether this node is a reload depot.
         */
        [[nodiscard]] inline bool isReloadDepot() const;

        /**
         * Assigns the node to the given route, at the given index, in the
         * given trip.
         */
        void assign(Route *route, size_t idx, size_t trip);

        /**
         * Removes the node from its assigned route, if any.
         */
        void unassign();
    };

    /**
     * Forward iterator through the client nodes visited by this route.
     */
    class Iterator
    {
        std::vector<Node *> const *nodes_;
        size_t idx_ = 0;

        // Ensures we skip reload depots.
        void ensureValidIndex();

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node *;

        Iterator(std::vector<Node *> const &nodes, size_t idx);

        Iterator() = default;
        Iterator(Iterator const &other) = default;
        Iterator(Iterator &&other) = default;

        Iterator &operator=(Iterator const &other) = default;
        Iterator &operator=(Iterator &&other) = default;

        bool operator==(Iterator const &other) const;

        Node *operator*() const;

        Iterator operator++(int);
        Iterator &operator++();
    };

private:
    using LoadSegments = std::vector<LoadSegment>;

    /**
     * Class storing data related to the route segment starting at ``start``,
     * and ending at the end depot (inclusive).
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
        inline Distance distance(size_t profile) const;
        inline DurationSegment duration(size_t profile) const;
        inline LoadSegment const &load(size_t dimension) const;
    };

    /**
     * Class storing data related to the route segment starting at the start
     * depot, and ending at ``end`` (inclusive).
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
        inline Distance distance(size_t profile) const;
        inline DurationSegment duration(size_t profile) const;
        inline LoadSegment const &load(size_t dimension) const;
    };

    /**
     * Class storing data related to the route segment starting at ``start``,
     * and ending at ``end`` (inclusive). The segment must consist of a single
     * trip, possibly including its ending depot.
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
        inline Distance distance(size_t profile) const;
        inline DurationSegment duration(size_t profile) const;
        inline LoadSegment load(size_t dimension) const;
    };

    ProblemData const &data;

    ProblemData::VehicleType const &vehicleType_;
    size_t const idx_;

    std::vector<Node> depots_;  // start, end, and reload depots (in that order)

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
    [[nodiscard]] inline Node const *operator[](size_t idx) const;

    [[nodiscard]] Iterator begin() const;
    [[nodiscard]] Iterator end() const;

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
     * True if this route has no client visits, false otherwise.
     */
    [[nodiscard]] inline bool empty() const;

    /**
     * Number of clients and depots on this route.
     */
    [[nodiscard]] inline size_t size() const;

    /**
     * Number of clients in this route.
     */
    [[nodiscard]] inline size_t numClients() const;

    /**
     * Returns the number of start, end, and reload depots in this route.
     */
    [[nodiscard]] inline size_t numDepots() const;

    /**
     * Returns the number of trips in this route.
     */
    [[nodiscard]] inline size_t numTrips() const;

    /**
     * Returns the maximum number of allowed trips for this route.
     */
    [[nodiscard]] inline size_t maxTrips() const;

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
     * returns true.
     */
    void clear();

    /**
     * Reserves capacity for at least given ``size`` number of nodes (depots
     * and clients).
     */
    void reserve(size_t size);

    /**
     * Inserts the given node before index ``idx``. Assumes the given index is
     * valid. Depot nodes are copied into internal memory, but of client nodes
     * no ownership is taken.
     */
    void insert(size_t idx, Node *node);

    /**
     * Appends the given node pointer at the end of the route. Depot nodes are
     * copied into internal memory, but of client nodes no ownership is taken.
     */
    void push_back(Node *node);

    /**
     * Removes the node at ``idx`` from the route. Start and end depots cannot
     * be removed.
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

size_t Route::Node::trip() const { return trip_; }

Route *Route::Node::route() const { return route_; }

bool Route::Node::isDepot() const
{
    return isStartDepot() || isEndDepot() || isReloadDepot();
}

bool Route::Node::isStartDepot() const
{
    return route_ && this == &route_->depots_[0];
}

bool Route::Node::isEndDepot() const
{
    return route_ && this == &route_->depots_[1];
}

bool Route::Node::isReloadDepot() const
{
    // clang-format off
    return route_
        && loc_ < route_->data.numDepots()
        && !isStartDepot()
        && !isEndDepot();
    // clang-format on
}

Route::SegmentAfter::SegmentAfter(Route const &route, size_t start)
    : route_(route), start(start)
{
    assert(start < route.size());
}

Route::SegmentBefore::SegmentBefore(Route const &route, size_t end)
    : route_(route), end(end)
{
    assert(end < route.size());
}

Route::SegmentBetween::SegmentBetween(Route const &route,
                                      size_t start,
                                      size_t end)
    : route_(route), start(start), end(end)
{
    assert(start <= end && end < route.size());

    // The segment must consist of a single trip only, possibly including the
    // depot that begins the next trip (and ends this one). So the difference
    // in trips is at most one.
    assert(route[end]->trip() - route[start]->trip() <= route[end]->isDepot());
}

Distance Route::SegmentAfter::distance([[maybe_unused]] size_t profile) const
{
    assert(profile == route_.profile());
    return {route_.cumDist.back() - route_.cumDist[start]};
}

DurationSegment
Route::SegmentAfter::duration([[maybe_unused]] size_t profile) const
{
    assert(profile == route_.profile());
#ifdef PYVRP_NO_TIME_WINDOWS
    return {};
#else
    return route_.durAfter[start];
#endif
}

LoadSegment const &Route::SegmentAfter::load(size_t dimension) const
{
    return route_.loadAfter[dimension][start];
}

Distance Route::SegmentBefore::distance([[maybe_unused]] size_t profile) const
{
    assert(profile == route_.profile());
    return route_.cumDist[end];
}

DurationSegment
Route::SegmentBefore::duration([[maybe_unused]] size_t profile) const
{
    assert(profile == route_.profile());
#ifdef PYVRP_NO_TIME_WINDOWS
    return {};
#else
    return route_.durBefore[end];
#endif
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

Distance Route::SegmentBetween::distance(size_t profile) const
{
    if (profile != route_.profile())  // then we have to compute the distance
    {                                 // segment from scratch.
        auto const &mat = route_.data.distanceMatrix(profile);
        Distance distance = 0;

        for (size_t step = start; step != end; ++step)
        {
            auto const from = route_.visits[step];
            auto const to = route_.visits[step + 1];
            distance += mat(from, to);
        }

        return distance;
    }

    auto const startDist = route_.cumDist[start];
    auto const endDist = route_.cumDist[end];

    assert(startDist <= endDist);
    return endDist - startDist;
}

DurationSegment
Route::SegmentBetween::duration([[maybe_unused]] size_t profile) const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return {};
#else
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
#endif
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

Route::Node const *Route::operator[](size_t idx) const
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
#ifdef PYVRP_NO_TIME_WINDOWS
    return 0;
#else
    return durAfter[0].duration();
#endif
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
#ifdef PYVRP_NO_TIME_WINDOWS
    return 0;
#else
    return durAfter[0].timeWarp(maxDuration());
#endif
}

size_t Route::profile() const { return vehicleType_.profile; }

bool Route::empty() const { return numClients() == 0; }

size_t Route::size() const { return nodes.size(); }

size_t Route::numClients() const { return size() - numDepots(); }

size_t Route::numDepots() const { return depots_.size(); }

size_t Route::numTrips() const { return depots_.size() - 1; }

size_t Route::maxTrips() const { return vehicleType_.maxTrips(); }

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

template <Segment... Segments>
Route::Proposal<Segments...>::Proposal(Segments &&...segments)
    : segments_(std::forward<Segments>(segments)...)
{
    static_assert(sizeof...(Segments) > 0, "Proposal cannot be empty.");

    [[maybe_unused]] auto &&first = std::get<0>(segments_);
    [[maybe_unused]] auto &&last = std::get<sizeof...(Segments) - 1>(segments_);
    assert(first.route() == last.route());  // must start and end at same route
}

template <Segment... Segments>
Route const *Route::Proposal<Segments...>::route() const
{
    return std::get<0>(segments_).route();
}

template <Segment... Segments>
Distance Route::Proposal<Segments...>::distance() const
{
    auto const &data = route()->data;
    auto const profile = route()->profile();
    auto const &matrix = data.distanceMatrix(profile);

    auto const fn = [&matrix, profile](auto &&segment, auto &&...args)
    {
        auto distance = segment.distance(profile);
        auto last = segment.last();

        auto const merge = [&](auto const &self, auto &&other, auto &&...args)
        {
            distance += matrix(last, other.first()) + other.distance(profile);
            last = other.last();

            if constexpr (sizeof...(args) != 0)
                self(self, std::forward<decltype(args)>(args)...);
        };

        merge(merge, std::forward<decltype(args)>(args)...);
        return distance;
    };

    return std::apply(fn, segments_);
}

template <Segment... Segments>
std::pair<Duration, Duration> Route::Proposal<Segments...>::duration() const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return std::make_pair(0, 0);
#else
    auto const &data = route()->data;
    auto const maxDuration = route()->maxDuration();
    auto const profile = route()->profile();
    auto const &matrix = data.durationMatrix(profile);

    // Finalising is expensive with duration segments. However, finaliseFront is
    // significantly less expensive than finaliseBack. To use it, we iterate the
    // segments in reverse (right to left, rather than default left to right).
    auto const fn = [&](auto &&segment, auto &&...args)
    {
        auto ds = segment.duration(profile);
        auto first = segment.first();

        if (first < data.numDepots())  // segment starts at depot
            ds = ds.finaliseFront();

        auto const merge = [&](auto const &self, auto &&other, auto &&...args)
        {
            auto edgeDur = matrix(other.last(), first);

            if (other.last() < data.numDepots())  // other ends at a depot
            {
                // We can only finalise the current segment at the depot, so we
                // first need to travel there.
                ProblemData::Depot const &depot = data.location(other.last());
                ds = DurationSegment::merge(edgeDur, {depot}, ds);
                ds = ds.finaliseFront();

                // We finalise by travelling to the depot, so the remaining
                // travel duration is now zero.
                edgeDur = 0;
            }

            ds = DurationSegment::merge(edgeDur, other.duration(profile), ds);
            first = other.first();

            if constexpr (sizeof...(args) != 0)
            {
                if (first < data.numDepots())  // other starts at a depot
                    ds = ds.finaliseFront();

                self(self, std::forward<decltype(args)>(args)...);
            }
        };

        merge(merge, std::forward<decltype(args)>(args)...);
        return std::make_pair(ds.duration(), ds.timeWarp(maxDuration));
    };

    return std::apply(fn, detail::reverse(segments_));
#endif  // PYVRP_NO_TIME_WINDOWS
}

template <Segment... Segments>
Load Route::Proposal<Segments...>::excessLoad(size_t dimension) const
{
    auto const &data = route()->data;
    auto const &capacities = route()->capacity();
    auto const capacity = capacities[dimension];

    auto const fn = [&](auto &&segment, auto &&...args)
    {
        auto ls = segment.load(dimension);
        if (segment.last() < data.numDepots())  // ends at depot
            ls = ls.finalise(capacity);

        auto const merge = [&](auto const &self, auto &&other, auto &&...args)
        {
            if (other.first() < data.numDepots())  // other starts at a depot
                ls = ls.finalise(capacity);

            ls = LoadSegment::merge(ls, other.load(dimension));

            if constexpr (sizeof...(args) != 0)
            {
                if (other.last() < data.numDepots())  // other ends at a depot
                    ls = ls.finalise(capacity);

                self(self, std::forward<decltype(args)>(args)...);
            }
        };

        merge(merge, std::forward<decltype(args)>(args)...);
        return ls.excessLoad(capacity);
    };

    return std::apply(fn, segments_);
}
}  // namespace pyvrp::search

// Outputs a route into a given ostream in human-readable format
std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route);

std::ostream &operator<<(std::ostream &out,  // for debugging
                         pyvrp::search::Route::Node const &node);

#endif  // PYVRP_SEARCH_ROUTE_H
