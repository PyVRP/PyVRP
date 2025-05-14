#ifndef PYVRP_TRIP_H
#define PYVRP_TRIP_H

#include "ProblemData.h"

#include <optional>
#include <vector>

namespace pyvrp
{
/**
 * Trip(
 *     data: ProblemData,
 *     visits: list[int],
 *     vehicle_type: int,
 *     start_depot: int | None = None,
 *     end_depot: int | None = None,
 * )
 *
 * A simple class that stores the trip plan and some related statistics. The
 * start and end depots default to the vehicle type's start and end depots if
 * not explicitly given.
 *
 * .. note::
 *
 *    A trip does not stand on its own - it is intended to be part of a
 *    :class:`~pyvrp._pyvrp.Route`, which tracks overall route statistics
 *    involving all trips, and determines route feasibility.
 */
class Trip
{
public:
    using Client = size_t;
    using Visits = std::vector<Client>;

private:
    Visits visits_;

    Distance distance_ = 0;         // Total travel distance on this trip
    std::vector<Load> delivery_;    // Total delivery amount served on this trip
    std::vector<Load> pickup_;      // Total pickup amount gathered on this trip
    std::vector<Load> load_;        // Load on this trip
    std::vector<Load> excessLoad_;  // Excess pickup or delivery demand
    Duration travel_ = 0;           // Total *travel* duration on this trip
    Duration service_ = 0;          // Total *service* duration on this trip
    Duration release_ = 0;          // Release time of this trip
    Cost prizes_ = 0;               // Total value of prizes on this trip

    std::pair<double, double> centroid_ = {0, 0};  // Trip center
    size_t vehicleType_;                           // Type of vehicle
    size_t startDepot_;                            // assigned start location
    size_t endDepot_;                              // assigned end location

public:
    [[nodiscard]] bool empty() const;

    /**
     * Returns the number of clients visited by this trip.
     */
    [[nodiscard]] size_t size() const;

    [[nodiscard]] Client operator[](size_t idx) const;

    [[nodiscard]] Visits::const_iterator begin() const;
    [[nodiscard]] Visits::const_iterator end() const;

    [[nodiscard]] Visits::const_reverse_iterator rbegin() const;
    [[nodiscard]] Visits::const_reverse_iterator rend() const;

    /**
     * Trip visits, as a list of clients.
     */
    [[nodiscard]] Visits const &visits() const;

    /**
     * Total distance travelled on this trip.
     */
    [[nodiscard]] Distance distance() const;

    /**
     * Total client delivery load on this trip.
     */
    [[nodiscard]] std::vector<Load> const &delivery() const;

    /**
     * Total client pickup load on this trip.
     */
    [[nodiscard]] std::vector<Load> const &pickup() const;

    /**
     * Maximum load at any point of this trip.
     */
    [[nodiscard]] std::vector<Load> const &load() const;

    /**
     * Pickup or delivery loads in excess of the vehicle's capacity.
     */
    [[nodiscard]] std::vector<Load> const &excessLoad() const;

    /**
     * Total duration of service on this trip.
     */
    [[nodiscard]] Duration serviceDuration() const;

    /**
     * Total duration of travel on this trip.
     */
    [[nodiscard]] Duration travelDuration() const;

    /**
     * Earliest time at which this trip can leave the depot. Follows from the
     * release times of clients visited on this trip.
     */
    [[nodiscard]] Duration releaseTime() const;

    /**
     * Total prize value collected on this trip.
     */
    [[nodiscard]] Cost prizes() const;

    /**
     * Center point of the client locations on this trip.
     */
    [[nodiscard]] std::pair<double, double> const &centroid() const;

    /**
     * Index of the type of vehicle used on this trip.
     */
    [[nodiscard]] size_t vehicleType() const;

    /**
     * Location index of the trip's starting depot.
     */
    [[nodiscard]] size_t startDepot() const;

    /**
     * Location index of the trip's ending depot.
     */
    [[nodiscard]] size_t endDepot() const;

    /**
     * Returns whether this trip violates capacity constraints.
     */
    [[nodiscard]] bool hasExcessLoad() const;

    bool operator==(Trip const &other) const;

    Trip &operator=(Trip const &other) = default;
    Trip &operator=(Trip &&other) = default;

    Trip() = delete;

    Trip(Trip const &other) = default;
    Trip(Trip &&other) = default;

    Trip(ProblemData const &data,
         Visits visits,
         size_t vehicleType,
         std::optional<size_t> startDepot = std::nullopt,
         std::optional<size_t> endDepot = std::nullopt);

    // This constructor does *no* validation. Useful when unserialising objects.
    Trip(Visits visits,
         Distance distance,
         std::vector<Load> delivery,
         std::vector<Load> pickup,
         std::vector<Load> load,
         std::vector<Load> excessLoad,
         Duration travel,
         Duration service,
         Duration release,
         Cost prizes,
         std::pair<double, double> centroid,
         size_t vehicleType,
         size_t startDepot,
         size_t endDepot);
};
}  // namespace pyvrp

std::ostream &operator<<(std::ostream &out, pyvrp::Trip const &trip);

#endif  // PYVRP_TRIP_H
