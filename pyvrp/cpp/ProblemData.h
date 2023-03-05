#ifndef HGS_PROBLEMDATA_H
#define HGS_PROBLEMDATA_H

#include "Matrix.h"
#include "XorShift128.h"
#include "precision.h"

#include <iosfwd>
#include <vector>

class ProblemData
{
public:
    struct Client
    {
        int x;                          // Coordinate X
        int y;                          // Coordinate Y
        duration_type serviceDuration;  // Service duration
        int demand;                     // Demand
        duration_type twEarly;  // Earliest arrival (when using time windows)
        duration_type twLate;   // Latest arrival (when using time windows)
    };

private:
    Matrix<distance_type> const dist_;  // Distance matrix (+depot)
    std::vector<Client> clients_;       // Client (+depot) information

    size_t const numClients_;
    size_t const numVehicles_;
    size_t const vehicleCapacity_;

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
     * @param first First client.
     * @param second Second client.
     * @return distance from the first to the second client.
     */
    [[nodiscard]] inline distance_type dist(size_t first, size_t second) const;

    /**
     * Returns the travel duration between the indicated two clients.
     *
     * @param first First client.
     * @param second Second client.
     * @return duration to travel from the first to the second client.
     */
    [[nodiscard]] inline duration_type duration(size_t first,
                                                size_t second) const;

    /**
     * @return The full distance matrix.
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
    [[nodiscard]] size_t vehicleCapacity() const;

    /**
     * Constructs a ProblemData object with the given data. Assumes the data
     * contains the depot, such that each vector is one longer than the number
     * of clients.
     *
     * @param coords       Coordinates as pairs of [x, y].
     * @param demands      Client demands.
     * @param numVehicles  Number of vehicles.
     * @param vehicleCap   Vehicle capacity.
     * @param timeWindows  Time windows as pairs of [early, late].
     * @param servDurs     Service durations.
     * @param distMat      Distance matrix.
     */
    ProblemData(
        std::vector<std::pair<int, int>> const &coords,
        std::vector<int> const &demands,
        size_t numVehicles,
        size_t vehicleCap,
        std::vector<std::pair<duration_type, duration_type>> const &timeWindows,
        std::vector<duration_type> const &servDurs,
        std::vector<std::vector<duration_type>> const &distMat);
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
    // TODO separate duration and distance
    return dist_(first, second);
}

#endif  // HGS_PROBLEMDATA_H
