#include "ProblemData.h"

#include <variant>
#include <vector>

namespace pyvrp
{
/**
 * Trip(
 *     data: ProblemData,
 *     visits: list[int],
 *     vehicle_type: int,
 *     start: Depot | Reload,
 *     end: Depot | Reload,
 *     after: Trip | None = None,
 * )
 *
 * TODO
 */
class Trip
{
public:
    using Client = size_t;
    using Depot = ProblemData::Depot;
    using Reload = ProblemData::VehicleType::Reload;
    using TripDelimiter = std::variant<Depot const *, Reload const *>;
    using Visits = std::vector<Client>;

private:
    Visits visits_;

    Distance distance_ = 0;         // Total travel distance on this trip
    Distance excessDistance_ = 0;   // Excess travel distance
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
    TripDelimiter start_;  // assigned start location (depot or reload)
    TripDelimiter end_;    // assigned end location (depot or reload)

public:
    [[nodiscard]] bool empty() const;

    /**
     * Returns the number of clients visited by this trip.
     */
    [[nodiscard]] size_t size() const;

    Trip(ProblemData const &data,
         Visits visits,
         size_t const vehicleType,
         TripDelimiter start,
         TripDelimiter end,
         Trip const *after = nullptr);
};
}  // namespace pyvrp
