#include "ProblemData.h"

#include <variant>
#include <vector>

namespace pyvrp
{
/**
 * Trip(
 *     data: ProblemData,
 *     visits: list[int],
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
    TripDelimiter start_;
    TripDelimiter end_;

public:
    // TODO

    Trip(ProblemData const &data,
         Visits visits,
         TripDelimiter start,
         TripDelimiter end,
         Trip const *after = nullptr);
};
}  // namespace pyvrp
