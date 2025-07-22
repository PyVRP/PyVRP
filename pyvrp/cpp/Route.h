#ifndef PYVRP_ROUTE_H
#define PYVRP_ROUTE_H

#include "Measure.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Trip.h"

#include <iosfwd>
#include <optional>
#include <vector>

namespace pyvrp
{
/**
 * Route(data: ProblemData, visits: list[int] | list[Trip], vehicle_type: int)
 *
 * A simple class that stores the route plan and some statistics. Internally,
 * a route consists of one or more :class:`~pyvrp._pyvrp.Trip` objects.
 */
class Route
{
    using Client = size_t;
    using Depot = size_t;
    using VehicleType = size_t;
    using Trips = std::vector<Trip>;
    using Visits = std::vector<Client>;

    // Validates the consistency of the constructed instance.
    void validate(ProblemData const &data) const;

    // Creates the data returned by ``schedule()``.
    void makeSchedule(ProblemData const &data);

public:
    /**
     * Forward iterator through the clients visited by this route.
     */
    class Iterator
    {
        Route const *route_ = nullptr;
        size_t trip_ = 0;
        size_t idx_ = 0;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Client;

        Iterator(Route const &route, size_t idx);

        Iterator() = default;
        Iterator(Iterator const &other) = default;
        Iterator(Iterator &&other) = default;

        Iterator &operator=(Iterator const &other) = default;
        Iterator &operator=(Iterator &&other) = default;

        bool operator==(Iterator const &other) const;

        Client operator*() const;

        Iterator operator++(int);
        Iterator &operator++();
    };

    /**
     * Simple object that stores some data about a client or depot visit.
     *
     * Attributes
     * ----------
     * location : int
     *     Index of the visited location (client or depot).
     * trip : int
     *     Index of the trip visiting this location.
     * start_service : int
     *     Time at which service begins.
     * end_service : int
     *     Time at which service completes.
     * service_duration : int
     *     Duration of the service.
     * wait_duration : int
     *     If the vehicle arrives early, this is the duration it has to wait
     *     until it can begin service.
     * time_warp : int
     *     If the vehicle arrives late, this is the duration it has to 'travel
     *     back in time' to begin service. Non-zero time warp indicates an
     *     infeasible route.
     */
    struct ScheduledVisit
    {
        size_t const location = 0;
        size_t const trip = 0;
        Duration const startService = 0;
        Duration const endService = 0;
        Duration const waitDuration = 0;
        Duration const timeWarp = 0;

        ScheduledVisit(size_t location,
                       size_t trip,
                       Duration startService,
                       Duration endService,
                       Duration waitDuration,
                       Duration timeWarp);

        [[nodiscard]] Duration serviceDuration() const;
    };

private:
    Trips trips_ = {};
    std::vector<ScheduledVisit> schedule_ = {};  // Client visit schedule data
    Distance distance_ = 0;        // Total travel distance on this route
    Cost distanceCost_ = 0;        // Total cost of travel distance
    Distance excessDistance_ = 0;  // Excess travel distance
    std::vector<Load> delivery_;   // Total delivery amount served on this route
    std::vector<Load> pickup_;     // Total pickup amount gathered on this route
    std::vector<Load> excessLoad_;  // Excess pickup or delivery demand
    Duration duration_ = 0;         // Total duration of this route
    Cost durationCost_ = 0;         // Total cost of route duration
    Duration timeWarp_ = 0;         // Total time warp on this route
    Duration travel_ = 0;           // Total *travel* duration on this route
    Duration service_ = 0;          // Total *service* duration on this route
    Duration startTime_ = 0;        // (earliest) start time of this route
    Duration slack_ = 0;            // Total time slack on this route
    Cost prizes_ = 0;               // Total value of prizes on this route
    Distance internalDistance_ = 0; // Total distance travelled on this route, excluding the first and last segment
    double constDistancePenalty_ = -1; // Constant distance penalty for the model (-1 means disabled)

    std::pair<double, double> centroid_;  // Route center
    VehicleType vehicleType_;             // Type of vehicle
    Depot startDepot_;                    // Assigned start depot
    Depot endDepot_;                      // Assigned end depot

public:
    [[nodiscard]] bool empty() const;

    /**
     * Returns the number of clients visited by this route.
     */
    [[nodiscard]] size_t size() const;

    /**
     * Returns the number of trips in this route.
     */
    [[nodiscard]] size_t numTrips() const;

    [[nodiscard]] Client operator[](size_t idx) const;

    [[nodiscard]] Iterator begin() const;
    [[nodiscard]] Iterator end() const;

    /**
     * Returns the trips that make up this route.
     */
    [[nodiscard]] Trips const &trips() const;

    /**
     * Returns the trip at the given index.
     */
    [[nodiscard]] Trip const &trip(size_t idx) const;

    /**
     * Route visits, as a list of clients.
     */
    [[nodiscard]] Visits visits() const;

    /**
     * Statistics about each visit and the overall route schedule. This includes
     * all client visits, but also starting and leaving depots.
     *
     * .. note::
     *
     *    The schedule assumes the route starts at :meth:`~start_time`. Starting
     *    later may be feasible, but shifts the schedule.
     */
    [[nodiscard]] std::vector<ScheduledVisit> const &schedule() const;

    /**
     * Total distance travelled on this route.
     */
    [[nodiscard]] Distance distance() const;

    /**
     * Total cost of the distance travelled on this route.
     */
    [[nodiscard]] Cost distanceCost() const;

    /**
     * Distance in excess of the vehicle's maximum distance constraint.
     */
    [[nodiscard]] Distance excessDistance() const;

    /**
     * Total client delivery load on this route.
     */
    [[nodiscard]] std::vector<Load> const &delivery() const;

    /**
     * Total client pickup load on this route.
     */
    [[nodiscard]] std::vector<Load> const &pickup() const;

    /**
     * Pickup or delivery loads in excess of the vehicle's capacity.
     */
    [[nodiscard]] std::vector<Load> const &excessLoad() const;

    /**
     * Total route duration, including travel, service and waiting time.
     */
    [[nodiscard]] Duration duration() const;

    /**
     * Total cost of the duration of this route.
     */
    [[nodiscard]] Cost durationCost() const;

    /**
     * Total duration of client service on this route.
     */
    [[nodiscard]] Duration serviceDuration() const;

    /**
     * Amount of time warp incurred on this route.
     */
    [[nodiscard]] Duration timeWarp() const;

    /**
     * Total duration of travel on this route.
     */
    [[nodiscard]] Duration travelDuration() const;

    /**
     * Total waiting duration on this route.
     */
    [[nodiscard]] Duration waitDuration() const;

    /**
     * Total distance travelled on this route, excluding the first and last segment.
     */
    [[nodiscard]] Distance internalDistance() const;

    /**
     * Start time of this route. This is the earliest possible time at which
     * the route can leave the depot and have a minimal duration and time warp.
     * If there is positive :meth:`~slack`, the start time can be delayed by at
     * most :meth:`~slack` time units without increasing the total (minimal)
     * route duration, or time warp.
     *
     * .. note::
     *
     *    It may be possible to leave before the start time (if the vehicle's
     *    time window allows for it). That will introduce additional waiting
     *    time, such that the route duration will then no longer be minimal.
     *    Delaying departure by more than :meth:`~slack` time units always
     *    increases time warp, which could turn the route infeasible.
     */
    [[nodiscard]] Duration startTime() const;

    /**
     * End time of the route. This is equivalent to
     * ``start_time + duration - time_warp``.
     */
    [[nodiscard]] Duration endTime() const;

    /**
     * Time by which departure from the depot can be delayed without resulting
     * in (additional) time warp or increased route duration.
     */
    [[nodiscard]] Duration slack() const;

    /**
     * Earliest time at which this route can leave the depot. Follows from the
     * release times of clients visited on the first trip of this route.
     *
     * .. note::
     *
     *    The route's release time should not be later than its start time,
     *    unless the route has time warp.
     */
    [[nodiscard]] Duration releaseTime() const;

    /**
     * Total prize value collected on this route.
     */
    [[nodiscard]] Cost prizes() const;

    /**
     * Center point of the client locations on this route.
     */
    [[nodiscard]] std::pair<double, double> const &centroid() const;

    /**
     * Index of the type of vehicle used on this route.
     */
    [[nodiscard]] VehicleType vehicleType() const;

    /**
     * Location index of the route's starting depot.
     */
    [[nodiscard]] Depot startDepot() const;

    /**
     * Location index of the route's ending depot.
     */
    [[nodiscard]] Depot endDepot() const;

    /**
     * Returns whether this route is feasible.
     */
    [[nodiscard]] bool isFeasible() const;

    /**
     * Returns whether this route violates capacity constraints.
     */
    [[nodiscard]] bool hasExcessLoad() const;

    /**
     * Returns whether this route violates maximum distance constraints.
     */
    [[nodiscard]] bool hasExcessDistance() const;

    /**
     * Returns whether this route violates time window or maximum duration
     * constraints.
     */
    [[nodiscard]] bool hasTimeWarp() const;

    bool operator==(Route const &other) const;

    Route &operator=(Route const &other) = default;
    Route &operator=(Route &&other) = default;

    Route() = delete;

    Route(Route const &other) = default;
    Route(Route &&other) = default;

    Route(ProblemData const &data, Trips trips, VehicleType vehicleType);

    Route(ProblemData const &data, Visits visits, VehicleType vehicleType);

    // This constructor does *no* validation. Useful when unserialising objects.
    Route(Trips trips,
          Distance distance,
          Cost distanceCost,
          Distance excessDistance,
          std::vector<Load> delivery,
          std::vector<Load> pickup,
          std::vector<Load> excessLoad,
          Duration duration,
          Cost durationCost,
          Duration timeWarp,
          Duration travel,
          Duration service,
          Duration startTime,
          Duration slack,
          Cost prizes,
          std::pair<double, double> centroid,
          VehicleType vehicleType,
          Depot startDepot,
          Depot endDepot,
          std::vector<ScheduledVisit> schedule);
};
}  // namespace pyvrp

std::ostream &operator<<(std::ostream &out, pyvrp::Route const &route);

#endif  // PYVRP_ROUTE_H
