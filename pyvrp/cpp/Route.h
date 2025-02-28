#ifndef PYVRP_ROUTE_H
#define PYVRP_ROUTE_H

#include "Measure.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"

#include <iosfwd>
#include <optional>
#include <vector>

namespace pyvrp
{
/**
 * Route(data: ProblemData, visits: list[int], vehicle_type: int)
 *
 * A simple class that stores the route plan and some statistics.
 */
class Route
{
public:
    /**
     * Simple object that stores some data about a client visit.
     *
     * Attributes
     * ----------
     * start_service : int
     *     Time at which the client service begins.
     * end_service : int
     *     Time at which the client service completes.
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
        Duration const startService = 0;
        Duration const endService = 0;
        Duration const waitDuration = 0;
        Duration const timeWarp = 0;

        ScheduledVisit(Duration startService,
                       Duration endService,
                       Duration waitDuration,
                       Duration timeWarp);

        [[nodiscard]] Duration serviceDuration() const;
    };

private:
    using Client = size_t;
    using Depot = size_t;
    using VehicleType = size_t;
    using Visits = std::vector<Client>;

    Visits visits_ = {};                         // Client visits on this route
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
    Duration wait_ = 0;             // Total *waiting* duration on this route
    Duration release_ = 0;          // Release time of this route
    Duration startTime_ = 0;        // (earliest) start time of this route
    Duration slack_ = 0;            // Total time slack on this route
    Cost prizes_ = 0;               // Total value of prizes on this route

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

    [[nodiscard]] Client operator[](size_t idx) const;

    Visits::const_iterator begin() const;
    Visits::const_iterator end() const;

    /**
     * Route visits, as a list of clients.
     */
    [[nodiscard]] Visits const &visits() const;

    /**
     * Statistics about each client visit and the overall route schedule.
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
     * Total duration of service on this route, at depots and clients.
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
     *  ``start_time + duration - time_warp``.
     */
    [[nodiscard]] Duration endTime() const;

    /**
     * Time by which departure from the depot can be delayed without resulting
     * in (additional) time warp or increased route duration.
     */
    [[nodiscard]] Duration slack() const;

    /**
     * Earliest time at which this route can leave the depot. Follows from the
     * release times of clients visited on this route.
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

    Route() = delete;

    Route(ProblemData const &data,
          Visits visits,
          VehicleType const vehicleType);

    // This constructor does *no* validation. Useful when unserialising objects.
    Route(Visits visits,
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
          Duration wait,
          Duration release,
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
