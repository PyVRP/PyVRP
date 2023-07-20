#ifndef PYVRP_PROBLEMDATA_H
#define PYVRP_PROBLEMDATA_H

#include "Matrix.h"
#include "Measure.h"
#include "XorShift128.h"

#include <iosfwd>
#include <vector>

namespace pyvrp
{
class ProblemData
{
public:
    /**
     * Client(
     *    x: int,
     *    y: int,
     *    demand: int = 0,
     *    service_duration: int = 0,
     *    tw_early: int = 0,
     *    tw_late: int = 0,
     *    release_time: int = 0,
     *    prize: int = 0,
     *    required: bool = True,
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
     * demand
     *     The amount this client's demanding. Default 0.
     * service_duration
     *     This client's service duration, that is, the amount of time we need
     *     to visit the client for. Service should start (but not necessarily
     *     end) within the [:py:attr:`~tw_early`, :py:attr:`~tw_late`] interval.
     *     Default 0.
     * tw_early
     *     Earliest time at which we can visit this client. Default 0.
     * tw_late
     *     Latest time at which we can visit this client. Default 0.
     * release_time
     *     Earliest time at which this client is released, that is, the earliest
     *     time at which a vehicle may leave the depot to visit this client.
     *     Default 0.
     * prize
     *     Prize collected by visiting this client. Default 0.
     * required
     *     Whether this client must be part of a feasible solution. Default
     *     True.
     */
    struct Client
    {
        Coordinate const x;
        Coordinate const y;
        Load const demand;
        Duration const serviceDuration;
        Duration const twEarly;      // Earliest possible start of service
        Duration const twLate;       // Latest possible start of service
        Duration const releaseTime;  // Earliest possible time to leave depot
        Cost const prize = 0;        // Prize for visiting this client
        bool const required = true;  // Must client be in solution?

        Client(Coordinate x,
               Coordinate y,
               Load demand = 0,
               Duration serviceDuration = 0,
               Duration twEarly = 0,
               Duration twLate = 0,
               Duration releaseTime = 0,
               Cost prize = 0,
               bool required = true);
    };

    struct VehicleType
    {
        Load const capacity;        // This type's vehicle capacity
        size_t const numAvailable;  // Available vehicles of this type
        size_t const depot = 0;     // Departure and return depot location
    };

private:
    std::pair<double, double> centroid_;           // Center of client locations
    Matrix<Distance> const dist_;                  // Distance matrix
    Matrix<Duration> const dur_;                   // Duration matrix
    std::vector<Client> const clients_;            // Client/depot information
    std::vector<VehicleType> const vehicleTypes_;  // Vehicle type information

    size_t const numClients_;
    size_t const numVehicleTypes_;
    size_t const numVehicles_;

public:
    /**
     * @param client Client whose data to return.
     * @return A struct containing the indicated client's information.
     */
    [[nodiscard]] inline Client const &client(size_t client) const;

    /**
     * @return Centroid of client locations.
     */
    [[nodiscard]] std::pair<double, double> const &centroid() const;

    /**
     * @param vehicleType Vehicle type whose data to return.
     * @return A struct containing the vehicle type's information.
     */
    [[nodiscard]] inline VehicleType const &
    vehicleType(size_t vehicleType) const;

    /**
     * Returns the distance between the indicated two clients.
     *
     * @param first  First client.
     * @param second Second client.
     * @return distance from the first to the second client.
     */
    [[nodiscard]] inline Distance dist(size_t first, size_t second) const;

    /**
     * Returns the travel duration between the indicated two clients.
     *
     * @param first  First client.
     * @param second Second client.
     * @return Travel duration from the first to the second client.
     */
    [[nodiscard]] inline Duration duration(size_t first, size_t second) const;

    /**
     * @return The full travel distance matrix.
     */
    [[nodiscard]] Matrix<Distance> const &distanceMatrix() const;

    /**
     * @return The full travel duration matrix.
     */
    [[nodiscard]] Matrix<Duration> const &durationMatrix() const;

    /**
     * @return Total number of clients in this instance.
     */
    [[nodiscard]] size_t numClients() const;

    /**
     * @return Total number of vehicle types in this instance.
     */
    [[nodiscard]] size_t numVehicleTypes() const;

    /**
     * @return Total number of vehicles available in this instance.
     */
    [[nodiscard]] size_t numVehicles() const;

    /**
     * Constructs a ProblemData object with the given data. Assumes the list of
     * clients contains the depot, such that each vector is one longer than the
     * number of clients.
     *
     * @param clients      List of clients (including depot at index 0).
     * @param vehicleTypes List of vehicle types.
     * @param distMat      Distance matrix.
     * @param durMat       Duration matrix.
     */
    ProblemData(std::vector<Client> const &clients,
                std::vector<VehicleType> const &vehicleTypes,
                Matrix<Distance> const distMat,
                Matrix<Duration> const durMat);
};

ProblemData::Client const &ProblemData::client(size_t client) const
{
    return clients_[client];
}

ProblemData::VehicleType const &
ProblemData::vehicleType(size_t vehicleType) const
{
    return vehicleTypes_[vehicleType];
}

Distance ProblemData::dist(size_t first, size_t second) const
{
    return dist_(first, second);
}

Duration ProblemData::duration(size_t first, size_t second) const
{
    return dur_(first, second);
}
}  // namespace pyvrp

#endif  // PYVRP_PROBLEMDATA_H
