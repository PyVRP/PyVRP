#ifndef PYVRP_ROUTE_H
#define PYVRP_ROUTE_H

#include "Activity.h"
#include "CostEvaluator.h"
#include "Measure.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"

#include <iosfwd>
#include <vector>

namespace pyvrp
{
/**
 * Route(data: ProblemData, activities: list[Activity], vehicle_type: int)
 *
 * A simple class that stores the route plan and some statistics.
 */
class Route
{
    using Client = size_t;
    using Depot = size_t;
    using VehicleType = size_t;
    using Activities = std::vector<Activity>;

    // Creates the data returned by ``schedule()``.
    void makeSchedule(ProblemData const &data);

public:
    /**
     * Simple object that stores some data about a route activity.
     *
     * Attributes
     * ----------
     * activity: Activity
     *     Route activity.
     * trip : int
     *     Trip index.
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
    class ScheduledVisit
    {
        Activity activity_;
        size_t trip_ = 0;
        Duration startService_ = 0;
        Duration endService_ = 0;
        Duration waitDuration_ = 0;
        Duration timeWarp_ = 0;

    public:
        ScheduledVisit(Activity activity,
                       size_t trip,
                       Duration startService,
                       Duration endService,
                       Duration waitDuration,
                       Duration timeWarp);

        [[nodiscard]] Activity activity() const;
        [[nodiscard]] size_t trip() const;
        [[nodiscard]] Duration startService() const;
        [[nodiscard]] Duration endService() const;
        [[nodiscard]] Duration serviceDuration() const;
        [[nodiscard]] Duration waitDuration() const;
        [[nodiscard]] Duration timeWarp() const;
    };

private:
    Activities activities_ = {};
    std::vector<ScheduledVisit> schedule_ = {};  // Activity schedule
    Distance distance_ = 0;        // Total travel distance on this route
    Cost distanceCost_ = 0;        // Total cost of travel distance
    Distance excessDistance_ = 0;  // Excess travel distance
    std::vector<Load> delivery_;   // Total delivery amount served on this route
    std::vector<Load> pickup_;     // Total pickup amount gathered on this route
    std::vector<Load> excessLoad_;  // Excess pickup or delivery demand
    Duration duration_ = 0;         // Total duration of this route
    Duration overtime_ = 0;         // Total overtime of this route
    Cost durationCost_ = 0;         // Total cost of route duration
    Duration timeWarp_ = 0;         // Total time warp on this route
    Duration travel_ = 0;           // Total *travel* duration on this route
    Duration service_ = 0;          // Total *service* duration on this route
    Duration startTime_ = 0;        // (earliest) start time of this route
    Duration releaseTime_ = 0;      // Release time of the first route trip
    Duration slack_ = 0;            // Total time slack on this route
    Cost fixedVehicleCost_ = 0;     // Fixed cost of vehicle used on this route
    Cost prizes_ = 0;               // Total value of prizes on this route

    VehicleType vehicleType_;  // Type of vehicle
    Depot startDepot_;         // Assigned start depot
    Depot endDepot_;           // Assigned end depot

public:
    [[nodiscard]] bool empty() const;

    /**
     * Returns the number of activities in this route.
     */
    [[nodiscard]] size_t size() const;

    /**
     * Returns the number of clients in this route.
     */
    [[nodiscard]] size_t numClients() const;

    /**
     * Returns the number of trips in this route.
     */
    [[nodiscard]] size_t numTrips() const;

    [[nodiscard]] Activity operator[](size_t idx) const;

    [[nodiscard]] Activities::const_iterator begin() const;
    [[nodiscard]] Activities::const_iterator end() const;

    /**
     * Route activities.
     */
    [[nodiscard]] Activities const &activities() const;

    /**
     * Statistics about each activity and the overall route schedule.
     *
     * .. note::
     *
     *    The schedule assumes the route starts at :meth:`~start_time`. Starting
     *    later may be feasible, but shifts the schedule.
     */
    [[nodiscard]] std::vector<ScheduledVisit> const &schedule() const;

    /**
     * The fixed cost of the vehicle servicing this route.
     */
    [[nodiscard]] Cost fixedVehicleCost() const;

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
     * Total route duration, including travel, service, waiting and overtime.
     */
    [[nodiscard]] Duration duration() const;

    /**
     * Overtime incurred on this route.
     */
    [[nodiscard]] Duration overtime() const;

    /**
     * Total cost of the duration of this route, including overtime.
     */
    [[nodiscard]] Cost durationCost() const;

    /**
     * Total duration of client and depot service on this route.
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

    Route(ProblemData const &data,
          std::vector<Client> visits,
          VehicleType vehicleType);

    Route(ProblemData const &data,
          Activities activities,
          VehicleType vehicleType);

    // This constructor does *no* validation. Useful when unserialising objects.
    Route(Activities activities,
          Distance distance,
          Cost distanceCost,
          Distance excessDistance,
          std::vector<Load> delivery,
          std::vector<Load> pickup,
          std::vector<Load> excessLoad,
          Duration duration,
          Duration overtime,
          Cost durationCost,
          Duration timeWarp,
          Duration travel,
          Duration service,
          Duration startTime,
          Duration releaseTime,
          Duration slack,
          Cost prizes,
          VehicleType vehicleType,
          Depot startDepot,
          Depot endDepot,
          std::vector<ScheduledVisit> schedule);
};

template <>  // specialisation for pyvrp::Route
Cost CostEvaluator::penalisedCost(Route const &route) const;
}  // namespace pyvrp

std::ostream &operator<<(std::ostream &out,
                         pyvrp::Route::ScheduledVisit const &visit);

std::ostream &operator<<(std::ostream &out, pyvrp::Route const &route);

#endif  // PYVRP_ROUTE_H
