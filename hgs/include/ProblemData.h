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

public:
    // TODO make members private and rethink interface
    Matrix<int> const dist_;  // Distance matrix (+depot)

    int const nbClients;        // Number of clients (excluding the depot)
    int const nbVehicles;       // Number of vehicles
    int const vehicleCapacity;  // Capacity limit

    std::vector<Client> clients;  // Client (+depot) information

    [[nodiscard]] int dist(size_t row, size_t col) const
    {
        return dist_(row, col);
    }

    template <typename... Args>
    [[nodiscard]] int
    dist(size_t first, size_t second, size_t third, Args... args) const
    {
        return dist_(first, second) + dist(second, third, args...);
    }

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
     * @param coords      Coordinates as pairs of [x, y].
     * @param demands     Client demands.
     * @param nbVehicles  Number of vehicles.
     * @param vehicleCap  Vehicle capacity.
     * @param timeWindows Time windows as pairs of [early, late].
     * @param servDurs    Service durations.
     * @param distMat     Distance matrix.
     * @param releases    Client release times.
     */
    ProblemData(std::vector<std::pair<int, int>> const &coords,
                std::vector<int> const &demands,
                int nbVehicles,
                int vehicleCap,
                std::vector<std::pair<int, int>> const &timeWindows,
                std::vector<int> const &servDurs,
                std::vector<std::vector<int>> const &distMat,
                std::vector<int> const &releases);
};

#endif  // HGS_PROBLEMDATA_H
