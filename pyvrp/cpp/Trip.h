#include "ProblemData.h"

#include <vector>

namespace pyvrp
{
/**
 * Trip(
 *     data: ProblemData,
 *     visits: list[int],
 *     vehicle_type: int,
 *     start_depot: int,
 *     end_depot: int,
 *     after: Trip | None = None,
 * )
 *
 * TODO
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
    std::vector<Load> excessLoad_;  // Excess pickup or delivery demand
    Duration duration_ = 0;         // Total duration of this trip
    Duration timeWarp_ = 0;         // Total time warp on this trip
    Duration travel_ = 0;           // Total *travel* duration on this trip
    Duration service_ = 0;          // Total *service* duration on this trip
    Duration wait_ = 0;             // Total *waiting* duration on this trip
    Duration release_ = 0;          // Release time of this trip
    Duration startTime_ = 0;        // (earliest) start time of this trip
    Duration slack_ = 0;            // Total time slack on this trip
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
     * Pickup or delivery loads in excess of the vehicle's capacity.
     */
    [[nodiscard]] std::vector<Load> const &excessLoad() const;

    /**
     * Total trip duration, including travel, service and waiting time.
     */
    [[nodiscard]] Duration duration() const;

    /**
     * Total duration of service on this trip.
     */
    [[nodiscard]] Duration serviceDuration() const;

    /**
     * Amount of time warp incurred on this trip.
     */
    [[nodiscard]] Duration timeWarp() const;

    /**
     * Total duration of travel on this trip.
     */
    [[nodiscard]] Duration travelDuration() const;

    /**
     * Total waiting duration on this trip.
     */
    [[nodiscard]] Duration waitDuration() const;

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
     * Returns whether this trip is feasible.
     */
    [[nodiscard]] bool isFeasible() const;

    /**
     * Returns whether this trip violates capacity constraints.
     */
    [[nodiscard]] bool hasExcessLoad() const;

    /**
     * Returns whether this trip violates time window constraints.
     */
    [[nodiscard]] bool hasTimeWarp() const;

    bool operator==(Trip const &other) const;

    Trip() = delete;

    Trip(ProblemData const &data,
         Visits visits,
         size_t const vehicleType,
         size_t const startDepot,
         size_t const endDepot,
         Trip const *after = nullptr);
};
}  // namespace pyvrp
