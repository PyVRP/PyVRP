#ifndef PYVRP_CLIENT_H
#define PYVRP_CLIENT_H

#include "Measure.h"

#include <limits>
#include <string>
#include <vector>

namespace pyvrp
{
/**
 * Client(
 *    location: int,
 *    delivery: list[int] = [],
 *    pickup: list[int] = [],
 *    service_duration: int = 0,
 *    tw_early: int = 0,
 *    tw_late: int = np.iinfo(np.int64).max,
 *    release_time: int = 0,
 *    prize: int = 0,
 *    required: bool = True,
 *    group: int | None = None,
 *    *,
 *    name: str = "",
 * )
 *
 * Simple data object storing all client data as properties. See also
 * :doc:`../setup/concepts` for further information about these properties.
 *
 * Parameters
 * ----------
 * location
 *     Physical location of this client.
 * delivery
 *     The amounts this client demands from the depot.
 * pickup
 *     The amounts this client ships back to the depot.
 * service_duration
 *     Amount of time a vehicle needs to spend at this client before
 *     resuming its route. Service should start (but not necessarily end)
 *     within the [:py:attr:`~tw_early`, :py:attr:`~tw_late`] interval.
 *     Default 0.
 * tw_early
 *     Earliest time at which this client may be visited to start service.
 *     Default 0.
 * tw_late
 *     Latest time at which this client may be visited to start service.
 *     Unconstrained if not provided.
 * release_time
 *     Earliest time at which this client is released, that is, the earliest
 *     time at which a vehicle may leave the depot on a trip to visit this
 *     client. Default 0.
 * prize
 *     Prize collected by visiting this client. Default 0. If this client
 *     is not required, the prize needs to be sufficiently large to offset
 *     any travel cost before this client will be visited in a solution.
 * required
 *     Whether this client must be part of a feasible solution. Default
 *     True. Make sure to also update the prize value when setting this
 *     argument to False.
 * group
 *     Indicates membership of the given client group, if any. By default
 *     clients are not part of any groups.
 * name
 *     Free-form name field for this client. Default empty.
 *
 * Attributes
 * ----------
 * location
 *     Physical location of this client.
 * delivery
 *     Client delivery amounts shipped from the depot.
 * pickup
 *     Client pickup amounts returned to the depot.
 * service_duration
 *     Amount of time a vehicle needs to spend at this client before
 *     resuming its route.
 * tw_early
 *     Earliest time at which this client may be visited to start service.
 * tw_late
 *     Latest time at which this client may be visited to start service.
 * release_time
 *     Earliest time at which a vehicle may leave the depot on a trip to
 *     visit this client.
 * prize
 *     Prize collected by visiting this client.
 * required
 *     Whether visiting this client is required.
 * group
 *     Indicates membership of the given client group, if any.
 * name
 *     Free-form name field for this client.
 */
struct Client
{
    size_t const location;
    Duration const serviceDuration;
    Duration const twEarly;  // Earliest possible start of service
    Duration const twLate;   // Latest possible start of service
    std::vector<Load> const delivery;
    std::vector<Load> const pickup;
    Duration const releaseTime;         // Earliest possible time to leave depot
    Cost const prize;                   // Prize for visiting this client
    bool const required;                // Must client be in solution?
    std::optional<size_t> const group;  // Optional client group membership
    char const *name;                   // Client name (for reference)

    Client(size_t location,
           std::vector<Load> delivery = {},
           std::vector<Load> pickup = {},
           Duration serviceDuration = 0,
           Duration twEarly = 0,
           Duration twLate = std::numeric_limits<Duration>::max(),
           Duration releaseTime = 0,
           Cost prize = 0,
           bool required = true,
           std::optional<size_t> group = std::nullopt,
           std::string name = "");

    bool operator==(Client const &other) const;

    Client(Client const &client);
    Client(Client &&client);

    Client &operator=(Client const &client) = delete;
    Client &operator=(Client &&client) = delete;

    ~Client();
};
}  // namespace pyvrp

#endif  // PYVRP_CLIENT_H
