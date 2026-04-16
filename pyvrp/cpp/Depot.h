#ifndef PYVRP_DEPOT_H
#define PYVRP_DEPOT_H

#include "Measure.h"

#include <limits>
#include <string>

namespace pyvrp
{
/**
 * Depot(
 *    location: int,
 *    tw_early: int = 0,
 *    tw_late: int = np.iinfo(np.int64).max,
 *    service_duration: int = 0,
 *    *,
 *    name: str = "",
 * )
 *
 * Simple data object storing all depot data as (read-only) properties.
 *
 * Parameters
 * ----------
 * location
 *     Physical location of this depot.
 * tw_early
 *     Opening time of this depot. Default 0.
 * tw_late
 *     Closing time of this depot. Default unconstrained.
 * service_duration
 *     Time it takes to e.g. load a vehicle at this depot, at the start of
 *     a trip. Default 0.
 * name
 *     Free-form name field for this depot. Default empty.
 *
 * Attributes
 * ----------
 * location
 *     Physical location of this depot.
 * tw_early
 *     Opening time of this depot.
 * tw_late
 *     Closing time of this depot.
 * service_duration
 *     Time it takes to e.g. load a vehicle at this depot, at the start of
 *     a trip.
 * name
 *     Free-form name field for this depot.
 */
struct Depot
{
    size_t const location;
    Duration const serviceDuration;
    Duration const twEarly;  // Depot opening time
    Duration const twLate;   // Depot closing time
    char const *name;        // Depot name (for reference)

    Depot(size_t location,
          Duration twEarly = 0,
          Duration twLate = std::numeric_limits<Duration>::max(),
          Duration serviceDuration = 0,
          std::string name = "");

    bool operator==(Depot const &other) const;

    Depot(Depot const &depot);
    Depot(Depot &&depot);

    Depot &operator=(Depot const &depot) = delete;
    Depot &operator=(Depot &&depot) = delete;

    ~Depot();
};
}  // namespace pyvrp

#endif  // PYVRP_DEPOT_H
