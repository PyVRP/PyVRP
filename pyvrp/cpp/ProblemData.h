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

    struct RouteData
    {
        size_t vehicleCapacity;  // Vehicle capacity
        inline bool operator==(RouteData const &other) const;
        inline bool operator!=(RouteData const &other) const;
    };

private:
    Matrix<int> const dist_;          // Distance matrix (+depot)
    Matrix<int> const dur_;           // Duration matrix (+depot)
    std::vector<Client> clients_;     // Client (+depot) information
    std::vector<RouteData> routes_;   // Routes information per route
    std::vector<size_t> routeTypes_;  // Type idx per route

    size_t const numClients_;
    size_t const numAvailableRoutes_;

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
     * @param route Route for which to return the data.
     * @return A struct containing the indicated route's information.
     */
    [[nodiscard]] inline RouteData const &routeData(size_t route) const;

    /**
     * @param route Route for which to return the route type.
     * @return RouteType index of the route type of the route.
     */
    [[nodiscard]] inline size_t const &routeType(size_t route) const;

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
     * @return Total number of routes available in this instance.
     */
    [[nodiscard]] size_t maxNumRoutes() const;

    /**
     * Constructs a ProblemData object with the given data. Assumes the list of
     * clients contains the depot, such that each vector is one longer than the
     * number of clients.
     *
     * @param clients      List of clients (including depot at index 0).
     * @param vehicleCapacities     Vehicle capacity for each route.
     * @param distMat      Distance matrix.
     * @param durMat       Duration matrix.
     */
    ProblemData(std::vector<Client> const &clients,
                std::vector<size_t> const &vehicleCapacities,
                std::vector<std::vector<int>> const &distMat,
                std::vector<std::vector<int>> const &durMat);
};

inline bool ProblemData::RouteData::operator==(RouteData const &other) const
{
    return vehicleCapacity == other.vehicleCapacity;
}

inline bool ProblemData::RouteData::operator!=(RouteData const &other) const
{
    return !(*this == other);
}

ProblemData::Client const &ProblemData::client(size_t client) const
{
    return clients_[client];
}

ProblemData::RouteData const &ProblemData::routeData(size_t route) const
{
    return routes_[route];
}

size_t const &ProblemData::routeType(size_t route) const
{
    return routeTypes_[route];
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
