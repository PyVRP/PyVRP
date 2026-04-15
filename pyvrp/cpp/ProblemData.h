#ifndef PYVRP_PROBLEMDATA_H
#define PYVRP_PROBLEMDATA_H

#include "Client.h"
#include "ClientGroup.h"
#include "Depot.h"
#include "Location.h"
#include "Matrix.h"
#include "Measure.h"
#include "VehicleType.h"

#include <cassert>
#include <vector>

namespace pyvrp
{
/**
 * ProblemData(
 *     locations: list[Location],
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
 *    arguments follow the order of the ``locations`` argument.
 *
 * Parameters
 * ----------
 * locations
 *     List of physical locations.
 * clients
 *     List of clients to visit.
 * depots
 *     List of depots. At least one depot must be passed.
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
 *     When the data references locations, clients, depots, etc. that do not
 *     exist because the referenced index is out of range.
 */
class ProblemData
{
    std::vector<Matrix<Distance>> const dists_;    // Distance matrices
    std::vector<Matrix<Duration>> const durs_;     // Duration matrices
    std::vector<Location> const locations_;        // Location information
    std::vector<Client> const clients_;            // Client information
    std::vector<Depot> const depots_;              // Depot information
    std::vector<VehicleType> const vehicleTypes_;  // Vehicle type information
    std::vector<ClientGroup> const groups_;        // Client groups

    size_t const numVehicles_;
    size_t const numLoadDimensions_;
    bool const hasTimeWindows_;

    // Validates the consistency of the constructed instance.
    void validate() const;

public:
    bool operator==(ProblemData const &other) const = default;

    /**
     * Returns a list of all locations in the problem instance.
     */
    [[nodiscard]] std::vector<Location> const &locations() const;

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
     * Returns the location at the given index.
     *
     * Parameters
     * ----------
     * location
     *     Location index whose information to retrieve.
     */
    [[nodiscard]] Location const &location(size_t location) const;

    /**
     * Returns the client at the given index.
     *
     * Parameters
     * ----------
     * client
     *     Client index whose information to retrieve.
     */
    [[nodiscard]] inline Client const &client(size_t client) const;

    /**
     * Returns the depot at the given index.
     *
     * Parameters
     * ----------
     * depot
     *     Depot index whose information to retrieve.
     */
    [[nodiscard]] inline Depot const &depot(size_t depot) const;

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
     * Determines whether any of the :meth:`~clients` or :meth:`~depots` in this
     * instance have nonstandard time windows, or if any :meth:`~vehicle_types`
     * have nonstandard shift time windows or latest start constraints.
     */
    [[nodiscard]] inline bool hasTimeWindows() const;

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
     * Number of locations in this problem instance.
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
     * locations
     *     Optional list of locations.
     * clients
     *     Optional list of clients.
     * depots
     *     Optional list of depots.
     * vehicle_types
     *     Optional list of vehicle types.
     * distance_matrices
     *     Optional distance matrices, one per routing profile.
     * duration_matrices
     *     Optional duration matrices, one per routing profile.
     * groups
     *     Optional client groups.
     *
     * Returns
     * -------
     * ProblemData
     *    A new ProblemData instance with possibly replaced data.
     */
    ProblemData replace(std::optional<std::vector<Location>> &locations,
                        std::optional<std::vector<Client>> &clients,
                        std::optional<std::vector<Depot>> &depots,
                        std::optional<std::vector<VehicleType>> &vehicleTypes,
                        std::optional<std::vector<Matrix<Distance>>> &distMats,
                        std::optional<std::vector<Matrix<Duration>>> &durMats,
                        std::optional<std::vector<ClientGroup>> &groups) const;

    ProblemData(std::vector<Location> locations,
                std::vector<Client> clients,
                std::vector<Depot> depots,
                std::vector<VehicleType> vehicleTypes,
                std::vector<Matrix<Distance>> distMats,
                std::vector<Matrix<Duration>> durMats,
                std::vector<ClientGroup> groups = {});

    ProblemData() = delete;
};

Client const &ProblemData::client(size_t client) const
{
    assert(client < numClients());
    return clients_[client];
}

Depot const &ProblemData::depot(size_t depot) const
{
    assert(depot < numDepots());
    return depots_[depot];
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

bool ProblemData::hasTimeWindows() const { return hasTimeWindows_; }
}  // namespace pyvrp

#endif  // PYVRP_PROBLEMDATA_H
