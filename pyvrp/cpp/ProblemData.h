#ifndef PYVRP_PROBLEMDATA_H
#define PYVRP_PROBLEMDATA_H

#include "Matrix.h"
#include "Measure.h"

#include <cassert>
#include <iosfwd>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace pyvrp
{
/**
 * ProblemData(
 *     clients: list[Client],
 *     locations: list[Location],
 *     vehicle_types: list[VehicleType],
 *     distance_matrices: list[numpy.ndarray[int]],
 *     duration_matrices: list[numpy.ndarray[int]],
 *     groups: list[ClientGroup] = [],
 * )
 *
 * Creates a problem data instance. This instance contains all information
 * needed to solve the vehicle routing problem.
 *
 * Parameters
 * ----------
 * clients
 *     List of clients to visit.
 * locations
 *     List of locations. At least one location must be passed.
 * vehicle_types
 *     List of vehicle types in the problem instance.
 * distance_matrices
 *     Distance matrices that give the travel distances between all locations.
 *     Each matrix corresponds to a routing profile.
 * duration_matrices
 *     Duration matrices that give the travel durations between all locations.
 *     Each matrix corresponds to a routing profile.
 * groups
 *     List of client groups. Client groups have certain restrictions - see the
 *     definition for details. By default there are no groups, and empty groups
 *     must not be passed.
 *
 * Raises
 * ------
 * ValueError
 *     When the data is inconsistent.
 * IndexError
 *     When the data references locations or groups that do not exist because
 *     the referenced index is out of range.
 */
class ProblemData
{
    // Validates the consistency of the constructed instance.
    void validate() const;

public:
    /**
     * Client(
     *    location: int,
     *    delivery: int = 0,
     *    pickup: int = 0,
     *    service_duration: int = 0,
     *    tw_early: int = 0,
     *    tw_late: int = np.iinfo(np.int64).max,
     *    release_time: int = 0,
     *    prize: int = 0,
     *    required: bool = True,
     *    group: Optional[int] = None,
     *    *,
     *    name: str = "",
     * )
     *
     * Simple data object storing all client data as (read-only) properties.
     *
     * Parameters
     * ----------
     * location
     *     Location of this client.
     * delivery
     *     The amount this client demands from the depot. Default 0.
     * pickup
     *     The amount this client ships back to the depot. Default 0.
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
     *     time at which a vehicle may leave the depot to visit this client.
     *     Default 0.
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
     *     Location index associated with this client.
     * delivery
     *     Client delivery amount, shipped from a depot.
     * pickup
     *     Client pickup amount, returned back to a depot.
     * service_duration
     *     Amount of time a vehicle needs to spend at this client before
     *     resuming its route.
     * tw_early
     *     Earliest time at which this client may be visited to start service.
     * tw_late
     *     Latest time at which this client may be visited to start service.
     * release_time
     *     Earliest time at which a vehicle may leave the depot to visit this
     *     client.
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
        Load const delivery;
        Load const pickup;
        Duration const serviceDuration;
        Duration const twEarly;      // Earliest possible start of service
        Duration const twLate;       // Latest possible start of service
        Duration const releaseTime;  // Earliest possible time to leave depot
        Cost const prize;            // Prize for visiting this client
        bool const required;         // Must client be in solution?
        std::optional<size_t> const group;  // Optional client group membership
        char const *name;                   // Client name (for reference)

        Client(size_t location,
               Load delivery = 0,
               Load pickup = 0,
               Duration serviceDuration = 0,
               Duration twEarly = 0,
               Duration twLate = std::numeric_limits<Duration>::max(),
               Duration releaseTime = 0,
               Cost prize = 0,
               bool required = true,
               std::optional<size_t> group = std::nullopt,
               std::string name = "");

        Client(Client const &client);
        Client(Client &&client);

        Client &operator=(Client const &client) = delete;
        Client &operator=(Client &&client) = delete;

        ~Client();
    };

    /**
     * ClientGroup(clients: list[int] = [], required: bool = True)
     *
     * A client group that imposes additional restrictions on visits to clients
     * in the group.
     *
     * .. note::
     *
     *    Only mutually exclusive client groups are supported for now.
     *
     * Parameters
     * ----------
     * clients
     *     The clients in the group.
     * required
     *     Whether visiting this client group is required.
     *
     * Attributes
     * ----------
     * clients
     *     The clients in the group.
     * required
     *     Whether visiting this client group is required.
     * mutually_exclusive
     *     When ``True``, exactly one of the clients in this group must be
     *     visited if the group is required, and at most one if the group is
     *     not required.
     *
     * Raises
     * ------
     * ValueError
     *     When the given clients contain duplicates, or when a client is added
     *     to the group twice.
     */
    class ClientGroup
    {
        std::vector<size_t> clients_;  // clients in this group

    public:
        bool const required;                  // is visiting the group required?
        bool const mutuallyExclusive = true;  // at most one visit in group?

        explicit ClientGroup(std::vector<size_t> clients = {},
                             bool required = true);

        ClientGroup(ClientGroup const &group) = default;
        ClientGroup(ClientGroup &&group) = default;

        ClientGroup &operator=(ClientGroup const &group) = delete;
        ClientGroup &operator=(ClientGroup &&group) = delete;

        bool empty() const;
        size_t size() const;

        std::vector<size_t>::const_iterator begin() const;
        std::vector<size_t>::const_iterator end() const;

        std::vector<size_t> const &clients() const;

        void addClient(size_t client);
        void clear();
    };

    /**
     * Location(
     *    x: int,
     *    y: int,
     *    *,
     *    name: str = "",
     * )
     *
     * Simple data object storing location data as (read-only) properties.
     *
     * Parameters
     * ----------
     * x
     *     Horizontal coordinate, that is, the 'x' part of the location's
     *     (x, y) coordinates.
     * y
     *     Vertical coordinate, that is, the 'y' part of the location's (x, y)
     *     coordinates.
     * name
     *     Free-form name field for this location. Default empty.
     *
     * Attributes
     * ----------
     * x
     *     Horizontal coordinate of this location.
     * y
     *     Vertical coordinate of this location.
     * name
     *     Free-form name field for this location.
     */
    struct Location
    {
        Coordinate const x;
        Coordinate const y;
        char const *name;  // Location name (for reference)

        Location(Coordinate x, Coordinate y, std::string name = "");

        Location(Location const &location);
        Location(Location &&location);

        Location &operator=(Location const &location) = delete;
        Location &operator=(Location &&location) = delete;

        ~Location();
    };

    /**
     * VehicleType(
     *     num_available: int = 1,
     *     capacity: int = 0,
     *     start_location: int = 0,
     *     end_location: int = 0,
     *     fixed_cost: int = 0,
     *     tw_early: int = 0,
     *     tw_late: int = np.iinfo(np.int64).max,
     *     max_duration: int = np.iinfo(np.int64).max,
     *     max_distance: int = np.iinfo(np.int64).max,
     *     unit_distance_cost: int = 1,
     *     unit_duration_cost: int = 0,
     *     profile: int = 0,
     *     *,
     *     name: str = "",
     * )
     *
     * Simple data object storing all vehicle type data as properties.
     *
     * Parameters
     * ----------
     * num_available
     *     Number of vehicles of this type that are available. Must be positive.
     *     Default 1.
     * capacity
     *     Capacity of this vehicle type. This is the maximum total delivery or
     *     pickup amount the vehicle can store along the route. Must be
     *     non-negative. Default 0.
     * start_location
     *     Index of the location where vehicles of this type start their
     *     routes. Default 0 (first location).
     * end_location
     *     Index of the location where vehicles of this type end their routes.
     *     Default 0 (first location).
     * fixed_cost
     *     Fixed cost of using a vehicle of this type. Default 0.
     * tw_early
     *     Start of the vehicle type's shift. Default 0.
     * tw_late
     *     End of the vehicle type's shift. Unconstrained if not provided.
     * max_duration
     *     Maximum route duration. Unconstrained if not explicitly provided.
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
     * name
     *     Free-form name field for this vehicle type. Default empty.
     *
     * Attributes
     * ----------
     * num_available
     *     Number of vehicles of this type that are available.
     * capacity
     *     Capacity (maximum total demand) of this vehicle type.
     * start_location
     *     Start location associated with these vehicles.
     * end_location
     *     End location associated with these vehicles.
     * fixed_cost
     *     Fixed cost of using a vehicle of this type.
     * tw_early
     *     Start of the vehicle type's shift, if specified.
     * tw_late
     *     End of the vehicle type's shift, if specified.
     * max_duration
     *     Maximum duration of the route this vehicle type is assigned to. This
     *     is a very large number when the maximum duration is unconstrained.
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
     * name
     *     Free-form name field for this vehicle type.
     */
    struct VehicleType
    {
        size_t const numAvailable;    // Available vehicles of this type
        size_t const startLocation;   // Departure location
        size_t const endLocation;     // Return location
        Load const capacity;          // This type's vehicle capacity
        Duration const twEarly;       // Start of shift
        Duration const twLate;        // End of shift
        Duration const maxDuration;   // Maximum route duration
        Distance const maxDistance;   // Maximum route distance
        Cost const fixedCost;         // Fixed cost of using this vehicle type
        Cost const unitDistanceCost;  // Variable cost per unit of distance
        Cost const unitDurationCost;  // Variable cost per unit of duration
        size_t const profile;         // Distance and duration profile
        char const *name;             // Type name (for reference)

        VehicleType(size_t numAvailable = 1,
                    Load capacity = 0,
                    size_t startlocation = 0,
                    size_t endlocation = 0,
                    Cost fixedCost = 0,
                    Duration twEarly = 0,
                    Duration twLate = std::numeric_limits<Duration>::max(),
                    Duration maxDuration = std::numeric_limits<Duration>::max(),
                    Distance maxDistance = std::numeric_limits<Distance>::max(),
                    Cost unitDistanceCost = 1,
                    Cost unitDurationCost = 0,
                    size_t profile = 0,
                    std::string name = "");

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
                            std::optional<Load> capacity,
                            std::optional<size_t> startlocation,
                            std::optional<size_t> endlocation,
                            std::optional<Cost> fixedCost,
                            std::optional<Duration> twEarly,
                            std::optional<Duration> twLate,
                            std::optional<Duration> maxDuration,
                            std::optional<Distance> maxDistance,
                            std::optional<Cost> unitDistanceCost,
                            std::optional<Cost> unitDurationCost,
                            std::optional<size_t> profile,
                            std::optional<std::string> name) const;
    };

private:
    std::pair<double, double> centroid_;           // Center of client locations
    std::vector<Matrix<Distance>> const dists_;    // Distance matrices
    std::vector<Matrix<Duration>> const durs_;     // Duration matrices
    std::vector<Client> const clients_;            // Client information
    std::vector<Location> const locations_;        // Location information
    std::vector<VehicleType> const vehicleTypes_;  // Vehicle type information
    std::vector<ClientGroup> const groups_;        // Client groups

    size_t const numVehicles_;

public:
    /**
     * Returns location data for the location at the given index.
     */
    [[nodiscard]] inline Location const &location(size_t location) const;

    /**
     * Returns client data for the client at the given index.
     */
    [[nodiscard]] inline Client const &client(size_t client) const;

    /**
     * Returns a list of all clients in the problem instance.
     */
    [[nodiscard]] std::vector<Client> const &clients() const;

    /**
     * Returns a list of all locations in the problem instance.
     */
    [[nodiscard]] std::vector<Location> const &locations() const;

    /**
     * Returns a list of all client groups in the problem instance.
     */
    [[nodiscard]] std::vector<ClientGroup> const &groups() const;

    /**
     * Returns a list of all vehicle types in the problem instance.
     */
    [[nodiscard]] std::vector<VehicleType> const &vehicleTypes() const;

    /**
     * Returns a list of all distance matrices in the problem instance.
     *
     * .. note::
     *
     *    This method returns a read-only view of the underlying data. No
     *    matrices are copied, but the resulting data cannot be modified in any
     *    way!
     */
    [[nodiscard]] std::vector<Matrix<Distance>> const &distanceMatrices() const;

    /**
     * Returns a list of all duration matrices in the problem instance.
     *
     * .. note::
     *
     *    This method returns a read-only view of the underlying data. No
     *    matrices are copied, but the resulting data cannot be modified in any
     *    way!
     */
    [[nodiscard]] std::vector<Matrix<Duration>> const &durationMatrices() const;

    /**
     * Center point of all client locations.
     */
    [[nodiscard]] std::pair<double, double> const &centroid() const;

    /**
     * Returns the client group at the given index.
     *
     * Parameters
     * ----------
     * group
     *     Group index whose information to retrieve.
     */
    [[nodiscard]] ClientGroup const &group(size_t group) const;

    /**
     * Returns vehicle type data for the given vehicle type.
     *
     * Parameters
     * ----------
     * vehicle_type
     *     Vehicle type number whose information to retrieve.
     */
    [[nodiscard]] VehicleType const &vehicleType(size_t vehicleType) const;

    /**
     * The full travel distance matrix associated with the given routing
     * profile.
     *
     * .. note::
     *
     *    This method returns a read-only view of the underlying data. No
     *    matrix is copied, but the resulting data cannot be modified in any
     *    way!
     *
     * Parameters
     * ----------
     * profile
     *     Routing profile whose associated distance matrix to retrieve.
     */
    [[nodiscard]] inline Matrix<Distance> const &
    distanceMatrix(size_t profile) const;

    /**
     * The full travel duration matrix associated with the given routing
     * profile.
     *
     * .. note::
     *
     *    This method returns a read-only view of the underlying data. No
     *    matrix is copied, but the resulting data cannot be modified in any
     *    way!
     *
     * Parameters
     * ----------
     * profile
     *     Routing profile whose associated duration matrix to retrieve.
     */
    [[nodiscard]] inline Matrix<Duration> const &
    durationMatrix(size_t profile) const;

    /**
     * Number of clients in this problem instance.
     */
    [[nodiscard]] size_t numClients() const;

    /**
     * Number of locations in this problem instance.
     */
    [[nodiscard]] size_t numlocations() const;

    /**
     * Number of client groups in this problem instance.
     */
    [[nodiscard]] size_t numGroups() const;

    /**
     * Number of vehicle types in this problem instance.
     */
    [[nodiscard]] size_t numVehicleTypes() const;

    /**
     * Number of vehicles in this problem instance.
     */
    [[nodiscard]] size_t numVehicles() const;

    /**
     * Number of routing profiles in this problem instance.
     */
    [[nodiscard]] size_t numProfiles() const;

    /**
     * Returns a new ProblemData instance with the same data as this instance,
     * except for the given parameters, which are used instead.
     *
     * Parameters
     * ----------
     * clients
     *    Optional list of clients.
     * locations
     *    Optional list of locations.
     * vehicle_types
     *    Optional list of vehicle types.
     * distance_matrices
     *    Optional distance matrices, one per routing profile.
     * duration_matrices
     *    Optional duration matrices, one per routing profile.
     * groups
     *    Optional client groups.
     *
     * Returns
     * -------
     * ProblemData
     *    A new ProblemData instance with possibly replaced data.
     * */
    ProblemData replace(std::optional<std::vector<Client>> &clients,
                        std::optional<std::vector<Location>> &locations,
                        std::optional<std::vector<VehicleType>> &vehicleTypes,
                        std::optional<std::vector<Matrix<Distance>>> &distMats,
                        std::optional<std::vector<Matrix<Duration>>> &durMats,
                        std::optional<std::vector<ClientGroup>> &groups) const;

    ProblemData(std::vector<Client> clients,
                std::vector<Location> locations,
                std::vector<VehicleType> vehicleTypes,
                std::vector<Matrix<Distance>> distMats,
                std::vector<Matrix<Duration>> durMats,
                std::vector<ClientGroup> groups = {});

    ProblemData() = delete;
};

ProblemData::Location const &ProblemData::location(size_t location) const
{
    assert(idx < numLocations());
    return locations_[idx];
}

ProblemData::Client const &ProblemData::client(size_t client) const
{
    assert(client < numClients());
    return clients_[client];
}

Matrix<Distance> const &ProblemData::distanceMatrix(size_t profile) const
{
    assert(profile < dists_.size());
    return dists_[profile];
}

Matrix<Duration> const &ProblemData::durationMatrix(size_t profile) const
{
    assert(profile < durs_.size());
    return durs_[profile];
}
}  // namespace pyvrp

#endif  // PYVRP_PROBLEMDATA_H
