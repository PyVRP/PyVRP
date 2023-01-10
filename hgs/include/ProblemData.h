#ifndef HGS_PROBLEMDATA_H
#define HGS_PROBLEMDATA_H

#include "Matrix.h"
#include "XorShift128.h"

#include <iosfwd>
#include <vector>

class ProblemData
{
    struct Client
    {
        int x;            // Coordinate X
        int y;            // Coordinate Y
        int servDur;      // Service duration
        int demand;       // Demand
        int twEarly;      // Earliest arrival (when using time windows)
        int twLate;       // Latest arrival (when using time windows)
        int releaseTime;  // Routes with this client cannot leave depot before
                          // this time
    };

    std::vector<Client> clients_;  // Client (+depot) information

    size_t const numClients_;
    size_t const numVehicles_;
    size_t const vehicleCapacity_;

public:
    Matrix<int> const dist_;  // Distance matrix (+depot) TODO make private

    [[nodiscard]] Client client(size_t client) const;

    [[nodiscard]] Client depot() const;

    [[nodiscard]] int dist(size_t row, size_t col) const;

    // TODO remove this template?
    template <typename... Args>
    [[nodiscard]] int
    dist(size_t first, size_t second, size_t third, Args... args) const
    {
        return dist(first, second) + dist(second, third, args...);
    }

    [[nodiscard]] size_t numClients() const;

    [[nodiscard]] size_t numVehicles() const;

    [[nodiscard]] size_t vehicleCapacity() const;

    /**
     * Constructs a ProblemData object from the data read (in VRPLIB format)
     * from the given instance path.
     *
     * @param instPath Path to the instance data.
     * @returns        The constructed object.
     */
    static ProblemData fromFile(std::string const &instPath);

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
     * @param releases     Client release times.
     */
    ProblemData(std::vector<std::pair<int, int>> const &coords,
                std::vector<int> const &demands,
                size_t numVehicles,
                size_t vehicleCap,
                std::vector<std::pair<int, int>> const &timeWindows,
                std::vector<int> const &servDurs,
                std::vector<std::vector<int>> const &distMat,
                std::vector<int> const &releases);
};

#endif  // HGS_PROBLEMDATA_H
