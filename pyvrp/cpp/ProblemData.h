#ifndef HGS_PROBLEMDATA_H
#define HGS_PROBLEMDATA_H

#include "Matrix.h"
#include "XorShift128.h"

#include <iosfwd>
#include <vector>

class ProblemData
{
public:
    struct Client
    {
        int x;                // Coordinate X
        int y;                // Coordinate Y
        int serviceDuration;  // Service duration
        int demand;           // Demand
        int twEarly;          // Earliest arrival (when using time windows)
        int twLate;           // Latest arrival (when using time windows)
    };

    struct Route
    {
        size_t vehicleCapacity;  // Vehicle capacity
        inline bool operator==(const Route &other) const
        {
            return vehicleCapacity == other.vehicleCapacity;
        }
        inline bool operator!=(const Route &other) const
        {
            return !(*this == other);
        }
    };

private:
    Matrix<int> const dist_;       // Distance matrix (+depot)
    std::vector<Client> clients_;  // Client (+depot) information
    std::vector<Route> routes_;    // Routes information

    size_t const numClients_;
    size_t const numRoutes_;

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
     * @param route Route whose data to return.
     * @return A struct containing the indicated route's information.
     */
    [[nodiscard]] inline Route const &route(size_t route) const;

    /**
     * Returns the distance between the indicated two clients.
     *
     * @param first First client.
     * @param second Second client.
     * @return distance from the first to the second client.
     */
    [[nodiscard]] inline int dist(size_t first, size_t second) const;

    /**
     * @return The full distance matrix.
     */
    [[nodiscard]] Matrix<int> const &distanceMatrix() const;

    /**
     * @return Total number of clients in this instance.
     */
    [[nodiscard]] size_t numClients() const;

    /**
     * @return Total number of vehicles available in this instance.
     */
    [[nodiscard]] size_t numVehicles() const;

    /**
     * Constructs a ProblemData object with the given data. Assumes the data
     * contains the depot, such that each vector is one longer than the number
     * of clients.
     *
     * @param coords       Coordinates as pairs of [x, y].
     * @param demands      Client demands.
     * @param vehicleCapacities     Vehicle capacity for each route.
     * @param timeWindows  Time windows as pairs of [early, late].
     * @param servDurs     Service durations.
     * @param distMat      Distance matrix.
     */
    ProblemData(std::vector<std::pair<int, int>> const &coords,
                std::vector<int> const &demands,
                std::vector<size_t> const &vehicleCapacities,
                std::vector<std::pair<int, int>> const &timeWindows,
                std::vector<int> const &servDurs,
                std::vector<std::vector<int>> const &distMat);
};

ProblemData::Client const &ProblemData::client(size_t client) const
{
    return clients_[client];
}

ProblemData::Route const &ProblemData::route(size_t route) const
{
    return routes_[route];
}

int ProblemData::dist(size_t first, size_t second) const
{
    return dist_(first, second);
}

#endif  // HGS_PROBLEMDATA_H
