#ifndef HGS_PROBLEMDATA_H
#define HGS_PROBLEMDATA_H

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
        int const x;                          // Coordinate X
        int const y;                          // Coordinate Y
        capacity_type const demand;           // Demand
        duration_type const serviceDuration;  // Service duration
        duration_type const twEarly;          // Earliest possible arrival
        duration_type const twLate;           // Latest possible arrival
        cost_type const prize = 0;            // Prize for visiting this client
        bool const required = true;           // Must client be in solution?

        Client(int x,
               int y,
               int demand = 0,
               int serviceDuration = 0,
               int twEarly = 0,
               int twLate = 0,
               int prize = 0,
               bool required = true);
    };

private:
    Matrix<distance_type> const dist_;  // Distance matrix (+depot)
    Matrix<duration_type> const dur_;   // Duration matrix (+depot)
    std::vector<Client> clients_;       // Client (+depot) information

    size_t const numClients_;
    size_t const numVehicles_;
    capacity_type const vehicleCapacity_;

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
     * Returns the distance between the indicated two clients.
     *
     * @param first  First client.
     * @param second Second client.
     * @return distance from the first to the second client.
     */
    [[nodiscard]] inline distance_type dist(size_t first, size_t second) const;

    /**
     * Returns the travel duration between the indicated two clients.
     *
     * @param first  First client.
     * @param second Second client.
     * @return Travel duration from the first to the second client.
     */
    [[nodiscard]] inline duration_type duration(size_t first,
                                                size_t second) const;

    /**
     * @return The full travel distance matrix.
     */
    [[nodiscard]] Matrix<distance_type> const &distanceMatrix() const;

    /**
     * @return The full travel duration matrix.
     */
    [[nodiscard]] Matrix<duration_type> const &durationMatrix() const;

    /**
     * @return Total number of clients in this instance.
     */
    [[nodiscard]] size_t numClients() const;

    /**
     * @return Total number of vehicles available in this instance.
     */
    [[nodiscard]] size_t numVehicles() const;

    /**
     * @return Capacity of each vehicle in this instance.
     */
    [[nodiscard]] capacity_type vehicleCapacity() const;

    /**
     * Constructs a ProblemData object with the given data. Assumes the list of
     * clients contains the depot, such that each vector is one longer than the
     * number of clients.
     *
     * @param clients      List of clients (including depot at index 0).
     * @param numVehicles  Number of vehicles.
     * @param vehicleCap   Vehicle capacity.
     * @param distMat      Distance matrix.
     * @param durMat       Duration matrix.
     */
    ProblemData(std::vector<Client> const &clients,
                size_t numVehicles,
                capacity_type vehicleCap,
                Matrix<distance_type> const distMat,
                Matrix<duration_type> const durMat);
};

ProblemData::Client const &ProblemData::client(size_t client) const
{
    return clients_[client];
}

distance_type ProblemData::dist(size_t first, size_t second) const
{
    return dist_(first, second);
}

duration_type ProblemData::duration(size_t first, size_t second) const
{
    return dur_(first, second);
}

#endif  // HGS_PROBLEMDATA_H
