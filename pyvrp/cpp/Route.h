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

    void validate(ProblemData const &data, Activities const &activities) const;
    void setSchedule(ProblemData const &data, Activities const &activities);
    void setDistance(ProblemData const &data);
    void setLoad(ProblemData const &data);
    void setOtherStatistics(ProblemData const &data);

public:
    /**
     * Route activity with schedule information.
     *
     * Attributes
     * ----------
     * type : ActivityType
     *     The type of activity, for example a depot or client visit.
     * idx : int
     *     The index of the activity corresponding to the activity type.
     * trip : int
     *     Trip index.
     * start_time : int
     *     Time at which this activity begins.
     * end_time : int
     *     Time at which this activity completes.
     * duration : int
     *     Activity duration.
     * wait_duration : int
     *     If the vehicle arrives early for this activity, this is the duration
     *     it has to wait until it can begin the activity.
     * time_warp : int
     *     If the vehicle arrives late, this is the duration it has to 'travel
     *     back in time' to begin the activity. Non-zero time warp indicates an
     *     infeasible route.
     */
    class ScheduledActivity : public Activity
    {
        size_t trip_ = 0;
        Duration startTime_ = 0;
        Duration endTime_ = 0;
        Duration waitDuration_ = 0;
        Duration timeWarp_ = 0;

    public:
        ScheduledActivity(Activity activity,
                          size_t trip,
                          Duration startTime,
                          Duration endTime,
                          Duration waitDuration,
                          Duration timeWarp);

        bool operator==(ScheduledActivity const &other) const = default;

        [[nodiscard]] size_t trip() const;
        [[nodiscard]] Duration startTime() const;
        [[nodiscard]] Duration endTime() const;
        [[nodiscard]] Duration duration() const;
        [[nodiscard]] Duration waitDuration() const;
        [[nodiscard]] Duration timeWarp() const;
    };

private:
    using Schedule = std::vector<ScheduledActivity>;

    Schedule schedule_ = {};       // Activity schedule
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
     * Returns the number of depots in this route.
     */
    [[nodiscard]] size_t numDepots() const;

    /**
     * Returns the number of trips in this route.
     */
    [[nodiscard]] size_t numTrips() const;

    [[nodiscard]] ScheduledActivity const &operator[](size_t idx) const;

    [[nodiscard]] Schedule::const_iterator begin() const;
    [[nodiscard]] Schedule::const_iterator end() const;

    /**
     * Statistics about each activity and the overall route schedule.
     *
     * .. note::
     *
     *    The schedule assumes the route starts at :meth:`~start_time`. Starting
     *    later may be feasible, but shifts the schedule.
     */
    [[nodiscard]] Schedule const &schedule() const;

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
     * Index of the route's starting depot.
     */
    [[nodiscard]] Depot startDepot() const;

    /**
     * Index of the route's ending depot.
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
          std::vector<Client> const &visits,
          VehicleType vehicleType);

    Route(ProblemData const &data,
          Activities const &activities,
          VehicleType vehicleType);

    // This constructor does *no* validation. Useful when unserialising objects.
    Route(Schedule schedule,
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
          VehicleType vehicleType);
};

template <>  // specialisation for pyvrp::Route
Cost CostEvaluator::penalisedCost(Route const &route) const;
}  // namespace pyvrp

std::ostream &operator<<(std::ostream &out, pyvrp::Route const &route);

#endif  // PYVRP_ROUTE_H
