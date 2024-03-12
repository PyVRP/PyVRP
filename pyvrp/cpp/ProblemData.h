#ifndef PYVRP_PROBLEMDATA_H
#define PYVRP_PROBLEMDATA_H

#include "Matrix.h"
#include "Measure.h"

#include <iosfwd>
#include <limits>
#include <optional>
#include <vector>

namespace pyvrp
{
/**
 * ProblemData(
 *     clients: list[Client],
 *     depots: list[Depot],
 *     vehicle_types: list[VehicleType],
 *     distance_matrix: numpy.ndarray[int],
 *     duration_matrix: numpy.ndarray[int],
 *     groups: list[ClientGroup] = [],
 * )
 *
 * Creates a problem data instance. This instance contains all information
 * needed to solve the vehicle routing problem.
 *
 * .. note::
 *
 *    The ``distance_matrix`` and ``duration_matrix`` arguments should have all
 *    depots in the lower indices, starting from index ``0``. See also the
 *    :meth:`~pyvrp._pyvrp.ProblemData.location` method for details.
 *
 * Parameters
 * ----------
 * clients
 *     List of clients to visit.
 * depots
 *     List of depots. At least one depot must be passed.
 * vehicle_types
 *     List of vehicle types in the problem instance.
 * distance_matrix
 *     A matrix that gives the travel distances between all locations: both
 *     depots and clients.
 * duration_matrix
 *     A matrix that gives the travel durations between all locations: both
 *     depots and clients.
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
 *     When the data references clients, depots, or groups that do not exist
 *     because the referenced index is out of range.
 */
class ProblemData
{
    // Validates the consistency of the constructed instance.
    void validate() const;

public:
    /**
     * Client(
     *    x: int,
     *    y: int,
     *    delivery: int = 0,
     *    pickup: int = 0,
     *    service_duration: int = 0,
     *    tw_early: int = 0,
     *    tw_late: int = np.iinfo(np.int64).max,
     *    release_time: int = 0,
     *    prize: int = 0,
     *    required: bool = True,
     *    group: Optional[int] = None,
     *    name: str = "",
     * )
     *
     * Simple data object storing all client data as (read-only) properties.
     *
     * Parameters
     * ----------
     * x
     *     Horizontal coordinate of this client, that is, the 'x' part of the
     *     client's (x, y) location tuple.
     * y
     *     Vertical coordinate of this client, that is, the 'y' part of the
     *     client's (x, y) location tuple.
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
     *     Prize collected by visiting this client. Default 0.
     * required
     *     Whether this client must be part of a feasible solution. Default
     *     True.
     * group
     *     Indicates membership of the given client group, if any. By default
     *     clients are not part of any groups.
     * name
     *     Free-form name field for this client. Default empty.
     *
     * Attributes
     * ----------
     * x
     *     Horizontal coordinate of this client.
     * y
     *     Vertical coordinate of this client.
     * delivery
     *     Client delivery amount, shipped from depot.
     * pickup
     *     Client pickup amount, returned back to depot.
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
        Coordinate const x;
        Coordinate const y;
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

        Client(Coordinate x,
               Coordinate y,
               Load delivery = 0,
               Load pickup = 0,
               Duration serviceDuration = 0,
               Duration twEarly = 0,
               Duration twLate = std::numeric_limits<Duration>::max(),
               Duration releaseTime = 0,
               Cost prize = 0,
               bool required = true,
               std::optional<size_t> group = std::nullopt,
               char const *name = "");

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
     * Depot(
     *    x: int,
     *    y: int,
     *    tw_early: int = 0,
     *    tw_late: int = np.iinfo(np.int64).max,
     *    name: str = "",
     * )
     *
     * Simple data object storing all depot data as (read-only) properties.
     *
     * Parameters
     * ----------
     * x
     *     Horizontal coordinate of this depot, that is, the 'x' part of the
     *     depot's (x, y) location tuple.
     * y
     *     Vertical coordinate of this depot, that is, the 'y' part of the
     *     depot's (x, y) location tuple.
     * tw_early
     *     Opening time of this depot. Default 0.
     * tw_late
     *     Closing time of this depot. Default unconstrained.
     * name
     *     Free-form name field for this depot. Default empty.
     *
     * Attributes
     * ----------
     * x
     *     Horizontal coordinate of this depot.
     * y
     *     Vertical coordinate of this depot.
     * tw_early
     *     Opening time of this depot.
     * tw_late
     *     Closing time of this depot.
     * name
     *     Free-form name field for this depot.
     */
    struct Depot
    {
        Coordinate const x;
        Coordinate const y;
        Duration const twEarly;  // Depot opening time
        Duration const twLate;   // Depot closing time
        char const *name;        // Depot name (for reference)

        Depot(Coordinate x,
              Coordinate y,
              Duration twEarly = 0,
              Duration twLate = std::numeric_limits<Duration>::max(),
              char const *name = "");

        Depot(Depot const &depot);
        Depot(Depot &&depot);

        Depot &operator=(Depot const &depot) = delete;
        Depot &operator=(Depot &&depot) = delete;

        ~Depot();
    };

    /**
     * VehicleType(
     *     num_available: int = 1,
     *     capacity: int = 0,
     *     depot: int = 0,
     *     fixed_cost: int = 0,
     *     tw_early: int = 0,
     *     tw_late: int = np.iinfo(np.int64).max,
     *     max_duration: int = np.iinfo(np.int64).max,
     *     max_distance: int = np.iinfo(np.int64).max,
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
     * depot
     *     Depot (location index) that vehicles of this type dispatch from, and
     *     return to at the end of their routes. Default 0 (first depot).
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
     * name
     *     Free-form name field for this vehicle type. Default empty.
     *
     * Attributes
     * ----------
     * num_available
     *     Number of vehicles of this type that are available.
     * capacity
     *     Capacity (maximum total demand) of this vehicle type.
     * depot
     *     Depot associated with these vehicles.
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
     * name
     *     Free-form name field for this vehicle type.
     */
    struct VehicleType
    {
        size_t const numAvailable;   // Available vehicles of this type
        size_t const depot;          // Departure and return depot location
        Load const capacity;         // This type's vehicle capacity
        Cost const fixedCost;        // Fixed cost of using this vehicle type
        Duration const twEarly;      // Start of shift
        Duration const twLate;       // End of shift
        Duration const maxDuration;  // Maximum route duration
        Distance const maxDistance;  // Maximum route distance
        char const *name;            // Type name (for reference)

        VehicleType(size_t numAvailable = 1,
                    Load capacity = 0,
                    size_t depot = 0,
                    Cost fixedCost = 0,
                    Duration twEarly = 0,
                    Duration twLate = std::numeric_limits<Duration>::max(),
                    Duration maxDuration = std::numeric_limits<Duration>::max(),
                    Distance maxDistance = std::numeric_limits<Distance>::max(),
                    char const *name = "");

        VehicleType(VehicleType const &vehicleType);
        VehicleType(VehicleType &&vehicleType);

        VehicleType &operator=(VehicleType const &vehicleType) = delete;
        VehicleType &operator=(VehicleType &&vehicleType) = delete;

        ~VehicleType();
    };

private:
    /**
     * Simple union type that distinguishes between client and depot locations.
     */
    union Location
    {
        Client const *client;
        Depot const *depot;

        inline operator Client const &() const;
        inline operator Depot const &() const;
    };

    std::pair<double, double> centroid_;           // Center of client locations
    Matrix<Distance> const dist_;                  // Distance matrix
    Matrix<Duration> const dur_;                   // Duration matrix
    std::vector<Client> const clients_;            // Client information
    std::vector<Depot> const depots_;              // Depot information
    std::vector<VehicleType> const vehicleTypes_;  // Vehicle type information
    std::vector<ClientGroup> const groups_;        // Client groups

    size_t const numVehicles_;

public:
    /**
     * Returns location data for the location at the given index. This can
     * be a depot or a client: a depot if the ``idx`` argument is smaller than
     * :py:attr:`~num_depots`, and a client if the ``idx`` is bigger than that.
     *
     * Parameters
     * ----------
     * idx
     *     Location index whose information to retrieve.
     */
    [[nodiscard]] inline Location location(size_t idx) const;

    /**
     * Returns a list of all clients in the problem instance.
     */
    [[nodiscard]] std::vector<Client> const &clients() const;

    /**
     * Returns a list of all depots in the problem instance.
     */
    [[nodiscard]] std::vector<Depot> const &depots() const;

    /**
     * Returns a list of all client groups in the problem instance.
     */
    [[nodiscard]] std::vector<ClientGroup> const &groups() const;

    /**
     * Returns a list of all vehicle types in the problem instance.
     */
    [[nodiscard]] std::vector<VehicleType> const &vehicleTypes() const;

    /**
     * Center point of all client locations (excluding depots).
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
     * Returns the travel distance between the first and second argument,
     * according to this instance's travel distance matrix.
     *
     * Parameters
     * ----------
     * first
     *     Client or depot number.
     * second
     *     Client or depot number.
     */
    [[nodiscard]] Distance dist(size_t first, size_t second) const;

    /**
     * Returns the travel duration between the first and second argument,
     * according to this instance's travel duration matrix.
     *
     * Parameters
     * ----------
     * first
     *     Client or depot number.
     * second
     *     Client or depot number.
     */
    [[nodiscard]] Duration duration(size_t first, size_t second) const;

    /**
     * The full travel distance matrix.
     *
     * .. note::
     *
     *    This method returns a read-only view of the underlying data. No
     *    matrix is copied, but the resulting data cannot be modified in any
     *    way!
     */
    [[nodiscard]] inline Matrix<Distance> const &distanceMatrix() const;

    /**
     * The full travel duration matrix.
     *
     * .. note::
     *
     *    This method returns a read-only view of the underlying data. No
     *    matrix is copied, but the resulting data cannot be modified in any
     *    way!
     */
    [[nodiscard]] inline Matrix<Duration> const &durationMatrix() const;

    /**
     * Number of clients in this problem instance.
     */
    [[nodiscard]] size_t numClients() const;

    /**
     * Number of depots in this problem instance.
     */
    [[nodiscard]] size_t numDepots() const;

    /**
     * Number of client groups in this problem instance.
     */
    [[nodiscard]] size_t numGroups() const;

    /**
     * Number of locations in this problem instance, that is, the number of
     * depots plus the number of clients in the instance.
     */
    [[nodiscard]] size_t numLocations() const;

    /**
     * Number of vehicle types in this problem instance.
     */
    [[nodiscard]] size_t numVehicleTypes() const;

    /**
     * Number of vehicles in this problem instance.
     */
    [[nodiscard]] size_t numVehicles() const;

    /**
     * Returns a new ProblemData instance with the same data as this instance,
     * except for the given parameters, which are used instead.
     *
     * Parameters
     * ----------
     * clients
     *    Optional list of clients.
     * depots
     *    Optional list of depots.
     * vehicle_types
     *    Optional list of vehicle types.
     * distance_matrix
     *    Optional distance matrix.
     * duration_matrix
     *    Optional duration matrix.
     * groups
     *    Optional client groups.
     *
     * Returns
     * -------
     * ProblemData
     *    A new ProblemData instance with possibly replaced data.
     * */
    ProblemData replace(std::optional<std::vector<Client>> &clients,
                        std::optional<std::vector<Depot>> &depots,
                        std::optional<std::vector<VehicleType>> &vehicleTypes,
                        std::optional<Matrix<Distance>> &distMat,
                        std::optional<Matrix<Duration>> &durMat,
                        std::optional<std::vector<ClientGroup>> &groups);

    ProblemData(std::vector<Client> clients,
                std::vector<Depot> depots,
                std::vector<VehicleType> vehicleTypes,
                Matrix<Distance> distMat,
                Matrix<Duration> durMat,
                std::vector<ClientGroup> groups = {});

    ProblemData() = delete;
};

ProblemData::Location::operator Client const &() const { return *client; }

ProblemData::Location::operator Depot const &() const { return *depot; }

ProblemData::Location ProblemData::location(size_t idx) const
{
    assert(idx < numLocations());
    return idx < depots_.size()
               ? Location{.depot = &depots_[idx]}
               : Location{.client = &clients_[idx - depots_.size()]};
}

Matrix<Distance> const &ProblemData::distanceMatrix() const { return dist_; }

Matrix<Duration> const &ProblemData::durationMatrix() const { return dur_; }
}  // namespace pyvrp

#endif  // PYVRP_PROBLEMDATA_H
