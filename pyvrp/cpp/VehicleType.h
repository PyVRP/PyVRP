#ifndef PYVRP_VEHICLETYPE_H
#define PYVRP_VEHICLETYPE_H

#include "Measure.h"

#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace pyvrp
{
/**
 * VehicleType(
 *     num_available: int = 1,
 *     capacity: list[int] = [],
 *     start_depot: int = 0,
 *     end_depot: int = 0,
 *     fixed_cost: int = 0,
 *     tw_early: int = 0,
 *     tw_late: int = np.iinfo(np.int64).max,
 *     shift_duration: int = np.iinfo(np.int64).max,
 *     max_distance: int = np.iinfo(np.int64).max,
 *     unit_distance_cost: int = 1,
 *     unit_duration_cost: int = 0,
 *     profile: int = 0,
 *     start_late: int | None = None,
 *     initial_load: list[int] = [],
 *     reload_depots: list[int] = [],
 *     max_reloads: int = np.iinfo(np.uint64).max,
 *     max_overtime: int = 0,
 *     unit_overtime_cost: int = 0,
 *     *,
 *     name: str = "",
 * )
 *
 * Simple data object storing all vehicle type data as properties. See also
 * :doc:`../setup/concepts` for further information about these properties.
 *
 * Parameters
 * ----------
 * num_available
 *     Number of vehicles of this type that are available. Must be positive.
 *     Default 1.
 * capacity
 *     Capacities of this vehicle type, per load dimension. This capacity is
 *     the maximum total delivery or pickup amount that the vehicle can
 *     store along the route.
 * start_depot
 *     Depot (index) where vehicles of this type start their routes.
 *     Defaults to the first depot.
 * end_depot
 *     Depot (index) where vehicles of this type end routes. Defaults to
 *     the first depot.
 * fixed_cost
 *     Fixed cost of using a vehicle of this type. Default 0.
 * tw_early
 *     Start of the vehicle type's shift. Default 0.
 * tw_late
 *     End of the vehicle type's shift. Unconstrained if not provided.
 * shift_duration
 *     Nominal maximum route duration. May be extended through overtime
 *     (see :py:attr:`~max_overtime`) at additional cost. Unconstrained if
 *     not explicitly provided.
 * max_distance
 *     Maximum route distance. Unconstrained if not explicitly provided.
 * unit_distance_cost
 *     Cost per unit of distance travelled by vehicles of this type. Default
 *     1.
 * unit_duration_cost
 *     Cost per unit of duration on routes serviced by vehicles of this
 *     type. Default 0.
 * profile
 *     This vehicle type's routing profile. Default 0, the first profile.
 * start_late
 *     Latest start of the vehicle type's shift. Unconstrained if not
 *     provided.
 * initial_load
 *     Load already on the vehicle that need to be dropped off at a depot.
 *     This load is present irrespective of any client visits. By default
 *     this value is zero, and the vehicle only considers loads from client
 *     visits.
 * reload_depots
 *     List of reload depots (indices) this vehicle may visit along its
 *     route, to empty and reload for subsequent client visits. Defaults to
 *     an empty list, in which case no reloads are allowed.
 * max_reloads
 *     Maximum number of reloads the vehicle may perform on a route.
 *     Unconstrained if not explicitly provided.
 * max_overtime
 *     Maximum allowed overtime, on top of the :py:attr:`~shift_duration`.
 *     Default 0, that is, overtime is not allowed.
 * unit_overtime_cost
 *     Cost of a unit of overtime. This is in addition to the regular
 *     :py:attr:`~unit_duration_cost` of route durations. Default 0.
 * name
 *     Free-form name field for this vehicle type. Default empty.
 *
 * Attributes
 * ----------
 * num_available
 *     Number of vehicles of this type that are available.
 * capacity
 *     Capacities of this vehicle type, per load dimension.
 * start_depot
 *     Start depot associated with these vehicles.
 * end_depot
 *     End depot associated with these vehicles.
 * fixed_cost
 *     Fixed cost of using a vehicle of this type.
 * tw_early
 *     Start of the vehicle type's shift, if specified.
 * tw_late
 *     End of the vehicle type's shift, if specified.
 * shift_duration
 *     Nominal maximum shift duration of the route this vehicle type is
 *     assigned to. Default unconstrained.
 * max_distance
 *     Maximum travel distance of the route this vehicle type is assigned
 *     to. This is a very large number when the maximum distance is
 *     unconstrained.
 * unit_distance_cost
 *     Cost per unit of distance travelled by vehicles of this type.
 * unit_duration_cost
 *     Cost per unit of duration on routes using vehicles of this type.
 * profile
 *     This vehicle type's routing profile.
 * start_late
 *     Latest start of the vehicle type's shift. This is equal to
 *     ``tw_late`` when the latest start is not constrained.
 * initial_load
 *     Load already on the vehicle that need to be dropped off at a depot.
 *     This load is present irrespective of any client visits.
 * reload_depots
 *     List of reload locations this vehicle may visit along it route, to
 *     empty and reload.
 * max_reloads
 *     Maximum number of reloads the vehicle may perform on a route.
 * max_overtime
 *     Maximum amount of allowed overtime, on top of the nominal
 *     :py:attr:`~shift_duration`.
 * unit_overtime_cost
 *     Additional cost of a unit of overtime.
 * max_duration
 *     Hard maximum route duration constraint, computed as the sum of
 *     :py:attr:`~shift_duration` and :py:attr:`~max_overtime`.
 * name
 *     Free-form name field for this vehicle type.
 */
struct VehicleType
{
    size_t const numAvailable;         // Available vehicles of this type
    size_t const startDepot;           // Departure depot location
    size_t const endDepot;             // Return depot location
    std::vector<Load> const capacity;  // This type's vehicle capacity
    Duration const twEarly;            // Start of shift
    Duration const twLate;             // End of shift
    Duration const shiftDuration;      // Nominal shift duration
    Distance const maxDistance;        // Maximum route distance
    Cost const fixedCost;              // Fixed cost of using this vehicle type
    Cost const unitDistanceCost;       // Variable cost per unit of distance
    Cost const unitDurationCost;       // Variable cost per unit of duration
    size_t const profile;              // Distance and duration profile
    Duration const startLate;          // Latest start of shift
    std::vector<Load> const initialLoad;     // Initially used capacity
    std::vector<size_t> const reloadDepots;  // Reload locations
    size_t const maxReloads;                 // Maximum number of reloads
    Duration const maxOvertime;              // Maximum allowed overtime
    Cost const unitOvertimeCost;             // Cost per unit of overtime
    Duration const maxDuration;  // Maximum route duration, incl. overtime
    char const *name;            // Type name (for reference)

    VehicleType(size_t numAvailable = 1,
                std::vector<Load> capacity = {},
                size_t startDepot = 0,
                size_t endDepot = 0,
                Cost fixedCost = 0,
                Duration twEarly = 0,
                Duration twLate = std::numeric_limits<Duration>::max(),
                Duration shiftDuration = std::numeric_limits<Duration>::max(),
                Distance maxDistance = std::numeric_limits<Distance>::max(),
                Cost unitDistanceCost = 1,
                Cost unitDurationCost = 0,
                size_t profile = 0,
                std::optional<Duration> startLate = std::nullopt,
                std::vector<Load> initialLoad = {},
                std::vector<size_t> reloadDepots = {},
                size_t maxReloads = std::numeric_limits<size_t>::max(),
                Duration maxOvertime = 0,
                Cost unitOvertimeCost = 0,
                std::string name = "");

    bool operator==(VehicleType const &other) const;

    VehicleType(VehicleType const &vehicleType);
    VehicleType(VehicleType &&vehicleType);

    VehicleType &operator=(VehicleType const &vehicleType) = delete;
    VehicleType &operator=(VehicleType &&vehicleType) = delete;

    ~VehicleType();

    /**
     * Returns a new ``VehicleType`` with the same data as this one, except
     * for the given parameters, which are used instead.
     */
    VehicleType replace(std::optional<size_t> numAvailable,
                        std::optional<std::vector<Load>> capacity,
                        std::optional<size_t> startDepot,
                        std::optional<size_t> endDepot,
                        std::optional<Cost> fixedCost,
                        std::optional<Duration> twEarly,
                        std::optional<Duration> twLate,
                        std::optional<Duration> shiftDuration,
                        std::optional<Distance> maxDistance,
                        std::optional<Cost> unitDistanceCost,
                        std::optional<Cost> unitDurationCost,
                        std::optional<size_t> profile,
                        std::optional<Duration> startLate,
                        std::optional<std::vector<Load>> initialLoad,
                        std::optional<std::vector<size_t>> reloadDepots,
                        std::optional<size_t> maxReloads,
                        std::optional<Duration> maxOvertime,
                        std::optional<Cost> unitOvertimeCost,
                        std::optional<std::string> name) const;

    /**
     * Returns the maximum number of trips these vehicle can execute.
     */
    size_t maxTrips() const;
};
}  // namespace pyvrp

#endif  // PYVRP_VEHICLETYPE_H
