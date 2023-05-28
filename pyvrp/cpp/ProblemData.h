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
        int x;                 // Coordinate X
        int y;                 // Coordinate Y
        int demand;            // Demand
        int serviceDuration;   // Service duration
        int twEarly;           // Earliest arrival (when using time windows)
        int twLate;            // Latest arrival (when using time windows)
        int prize = 0;         // Prize collected when visiting this client
        bool required = true;  // Must this client be part of a solution?

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
    Matrix<int> const dist_;       // Distance matrix (+depot)
    Matrix<int> const dur_;        // Duration matrix (+depot)
    std::vector<Client> clients_;  // Client (+depot) information

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
     * @param first  First client.
     * @param second Second client.
     * @return distance from the first to the second client.
     */
    [[nodiscard]] inline int dist(size_t first, size_t second) const;

    /**
     * Returns the travel duration between the indicated two clients.
     *
     * @param first  First client.
     * @param second Second client.
     * @return Travel duration from the first to the second client.
     */
    [[nodiscard]] inline int duration(size_t first, size_t second) const;

    /**
     * @return The full travel distance matrix.
     */
    [[nodiscard]] Matrix<int> const &distanceMatrix() const;

    /**
     * @return The full travel duration matrix.
     */
    [[nodiscard]] Matrix<int> const &durationMatrix() const;

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
                size_t vehicleCap,
                std::vector<std::vector<int>> const &distMat,
                std::vector<std::vector<int>> const &durMat);
};

ProblemData::Client const &ProblemData::client(size_t client) const
{
    return clients_[client];
}

int ProblemData::dist(size_t first, size_t second) const
{
    return dist_(first, second);
}

int ProblemData::duration(size_t first, size_t second) const
{
    return dur_(first, second);
}

#endif  // HGS_PROBLEMDATA_H
