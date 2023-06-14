#ifndef PYVRP_PROBLEMDATA_H
#define PYVRP_PROBLEMDATA_H

#include "Matrix.h"
#include "Measure.h"
#include "XorShift128.h"

#include <iosfwd>
#include <vector>

class ProblemData
{
public:
    struct Client
    {
        Coordinate const x;
        Coordinate const y;
        Load const demand;
        Duration const serviceDuration;
        Duration const twEarly;      // Earliest possible start of service
        Duration const twLate;       // Latest possible start of service
        Cost const prize = 0;        // Prize for visiting this client
        bool const required = true;  // Must client be in solution?

        Client(Coordinate x,
               Coordinate y,
               Load demand = 0,
               Duration serviceDuration = 0,
               Duration twEarly = 0,
               Duration twLate = 0,
               Cost prize = 0,
               bool required = true);
    };

    struct VehicleType
    {
        Load capacity;        // Capacity (max total demand) of the vehicle
        size_t numAvailable;  // Qty available of this vehicle type
        inline bool operator==(VehicleType const &other) const;
        inline bool operator!=(VehicleType const &other) const;
    };

private:
    std::pair<double, double> centroid_;     // Centroid of client locations
    Matrix<Distance> const dist_;            // Distance matrix (+depot)
    Matrix<Duration> const dur_;             // Duration matrix (+depot)
    std::vector<Client> clients_;            // Client (+depot) information
    std::vector<VehicleType> vehicleTypes_;  // Routes information per route

    size_t const numClients_;
    size_t const numVehicles_;
    size_t const numVehicleTypes_;

public:
    /**
     * @param client Client whose data to return.
     * @return A struct containing the indicated client's information.
     */
    [[nodiscard]] inline Client const &client(size_t client) const;

    /**
     * @return A struct containing the depot's information.
     */
    [[nodiscard]] Client const &depot() const;

    /**
     * @return Centroid of client locations.
     */
    [[nodiscard]] std::pair<double, double> const &centroid() const;

    /**
     * @param idx Index of the vehicle type to return.
     * @return A struct containing the vehicle type.
     */
    [[nodiscard]] inline VehicleType const &vehicleType(size_t idx) const;

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
     * @return Total number of routes available in this instance.
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

inline bool ProblemData::VehicleType::operator==(VehicleType const &other) const
{
    return capacity == other.capacity;
}

inline bool ProblemData::VehicleType::operator!=(VehicleType const &other) const
{
    return !(*this == other);
}

ProblemData::Client const &ProblemData::client(size_t client) const
{
    return clients_[client];
}

ProblemData::VehicleType const &ProblemData::vehicleType(size_t idx) const
{
    return vehicleTypes_[idx];
}

Distance ProblemData::dist(size_t first, size_t second) const
{
    return dist_(first, second);
}

Duration ProblemData::duration(size_t first, size_t second) const
{
    return dur_(first, second);
}

#endif  // PYVRP_PROBLEMDATA_H
