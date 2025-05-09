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
 *     depots: list[Depot],
 *     vehicle_types: list[VehicleType],
 *     distance_matrices: list[numpy.ndarray[int]],
 *     duration_matrices: list[numpy.ndarray[int]],
 *     groups: list[ClientGroup] = [],
 * )
 *
 * Creates a problem data instance. This instance contains all information
 * needed to solve the vehicle routing problem.
 *
 * .. note::
 *
 *    The matrices in the ``distance_matrices`` and ``duration_matrices``
 *    arguments should have all depots in the lower indices, starting from
 *    index ``0``. See also the :meth:`~pyvrp._pyvrp.ProblemData.location`
 *    method for details.
 *
 * Parameters
 * ----------
 * clients
 *     List of clients to visit.
 * depots
 *     List of depots. At least one depot must be passed.
 * vehicle_types
 *     List of vehicle types in the problem instance.
 * distance_matrices
 *     Distance matrices that give the travel distances between all locations
 *     (both depots and clients). Each matrix corresponds to a routing profile.
 * duration_matrices
 *     Duration matrices that give the travel durations between all locations
 *     (both depots and clients). Each matrix corresponds to a routing profile.
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
     * x
     *     Horizontal coordinate of this client, that is, the 'x' part of the
     *     client's (x, y) location tuple.
     * y
     *     Vertical coordinate of this client, that is, the 'y' part of the
     *     client's (x, y) location tuple.
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
     * x
     *     Horizontal coordinate of this client.
     * y
     *     Vertical coordinate of this client.
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
        Coordinate const x;
        Coordinate const y;
        Duration const serviceDuration;
        Duration const twEarly;  // Earliest possible start of service
        Duration const twLate;   // Latest possible start of service
        std::vector<Load> const delivery;
        std::vector<Load> const pickup;
        Duration const releaseTime;  // Earliest possible time to leave depot
        Cost const prize;            // Prize for visiting this client
        bool const required;         // Must client be in solution?
        std::optional<size_t> const group;  // Optional client group membership
        char const *name;                   // Client name (for reference)

        Client(Coordinate x,
               Coordinate y,
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

        bool operator==(ClientGroup const &other) const = default;

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
     *    *,
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
              std::string name = "");

        bool operator==(Depot const &other) const;

        Depot(Depot const &depot);
        Depot(Depot &&depot);

        Depot &operator=(Depot const &depot) = delete;
        Depot &operator=(Depot &&depot) = delete;

        ~Depot();
    };

    /**
     * VehicleType(
     *     num_available: int = 1,
     *     capacity: list[int] = [],
     *     start_depot: int = 0,
     *     end_depot: int = 0,
     *     fixed_cost: int = 0,
     *     tw_early: int = 0,
     *     tw_late: int = np.iinfo(np.int64).max,
     *     max_duration: int = np.iinfo(np.int64).max,
     *     max_distance: int = np.iinfo(np.int64).max,
     *     unit_distance_cost: int = 1,
     *     unit_duration_cost: int = 0,
     *     profile: int = 0,
     *     start_late: int | None = None,
     *     initial_load: list[int] = [],
     *     reload_depots: list[int] = [],
     *     max_reloads: int = np.iinfo(np.uint64).max,
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
     *     Depot (location index) where vehicles of this type start their
     *     routes. Default 0 (first depot).
     * end_depot
     *     Depot (location index) where vehicles of this type end routes.
     *     Default 0 (first depot).
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
     * start_late
     *     Latest start of the vehicle type's shift. Unconstrained if not
     *     provided.
     * initial_load
     *     Load already on the vehicle that need to be dropped off at a depot.
     *     This load is present irrespective of any client visits. By default
     *     this value is zero, and the vehicle only considers loads from client
     *     visits.
     * reload_depots
     *     List of reload depots (location indices) this vehicle may visit along
     *     its route, to empty and reload for subsequent client visits. Defaults
     *     to an empty list, in which case no reloads are allowed.
     * max_reloads
     *     Maximum number of reloads the vehicle may perform on a route.
     *     Unconstrained if not explicitly provided.
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
     *     Start location associated with these vehicles.
     * end_depot
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
        Duration const maxDuration;        // Maximum route duration
        Distance const maxDistance;        // Maximum route distance
        Cost const fixedCost;         // Fixed cost of using this vehicle type
        Cost const unitDistanceCost;  // Variable cost per unit of distance
        Cost const unitDurationCost;  // Variable cost per unit of duration
        size_t const profile;         // Distance and duration profile
        Duration const startLate;     // Latest start of shift
        std::vector<Load> const initialLoad;     // Initially used capacity
        std::vector<size_t> const reloadDepots;  // Reload locations
        size_t const maxReloads;                 // Maximum number of reloads
        char const *name;                        // Type name (for reference)

        VehicleType(size_t numAvailable = 1,
                    std::vector<Load> capacity = {},
                    size_t startDepot = 0,
                    size_t endDepot = 0,
                    Cost fixedCost = 0,
                    Duration twEarly = 0,
                    Duration twLate = std::numeric_limits<Duration>::max(),
                    Duration maxDuration = std::numeric_limits<Duration>::max(),
                    Distance maxDistance = std::numeric_limits<Distance>::max(),
                    Cost unitDistanceCost = 1,
                    Cost unitDurationCost = 0,
                    size_t profile = 0,
                    std::optional<Duration> startLate = std::nullopt,
                    std::vector<Load> initialLoad = {},
                    std::vector<size_t> reloadDepots = {},
                    size_t maxReloads = std::numeric_limits<size_t>::max(),
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
                            std::optional<Duration> maxDuration,
                            std::optional<Distance> maxDistance,
                            std::optional<Cost> unitDistanceCost,
                            std::optional<Cost> unitDurationCost,
                            std::optional<size_t> profile,
                            std::optional<Duration> startLate,
                            std::optional<std::vector<Load>> initialLoad,
                            std::optional<std::vector<size_t>> reloadDepots,
                            std::optional<size_t> maxReloads,
                            std::optional<std::string> name) const;

        /**
         * Returns the maximum number of trips these vehicle can execute.
         */
        size_t maxTrips() const;
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
    std::vector<Matrix<Distance>> const dists_;    // Distance matrices
    std::vector<Matrix<Duration>> const durs_;     // Duration matrices
    std::vector<Client> const clients_;            // Client information
    std::vector<Depot> const depots_;              // Depot information
    std::vector<VehicleType> const vehicleTypes_;  // Vehicle type information
    std::vector<ClientGroup> const groups_;        // Client groups

    size_t const numVehicles_;
    size_t const numLoadDimensions_;

public:
    bool operator==(ProblemData const &other) const = default;

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
     * Number of routing profiles in this problem instance.
     */
    [[nodiscard]] size_t numProfiles() const;

    /**
     * Number of load dimensions in this problem instance.
     */
    [[nodiscard]] size_t numLoadDimensions() const;

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
     */
    ProblemData replace(std::optional<std::vector<Client>> &clients,
                        std::optional<std::vector<Depot>> &depots,
                        std::optional<std::vector<VehicleType>> &vehicleTypes,
                        std::optional<std::vector<Matrix<Distance>>> &distMats,
                        std::optional<std::vector<Matrix<Duration>>> &durMats,
                        std::optional<std::vector<ClientGroup>> &groups) const;

    ProblemData(std::vector<Client> clients,
                std::vector<Depot> depots,
                std::vector<VehicleType> vehicleTypes,
                std::vector<Matrix<Distance>> distMats,
                std::vector<Matrix<Duration>> durMats,
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
