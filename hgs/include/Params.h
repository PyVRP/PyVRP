#ifndef PARAMS_H
#define PARAMS_H

#include "Config.h"
#include "Matrix.h"
#include "XorShift128.h"

#include <iosfwd>
#include <vector>

// Class that stores all the parameters (from the command line) (in Config) and
// data of the instance needed by the algorithm
class Params
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
    // TODO make members private
    Matrix<int> dist_;  // Distance matrix (+depot)

    Config const config;  // Stores all the parameter values

    int penaltyCapacity;  // Excess capacity penalty (per unit)
    int penaltyTimeWarp;  // Time warp penalty (per unit)

    int nbClients;        // Number of clients (excluding the depot)
    int nbVehicles;       // Number of vehicles
    int vehicleCapacity;  // Capacity limit

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
     * Constructs a Params object from the given configuration and data read
     * (in VRPLIB format) from the given instance path.
     *
     * @param config   Configuration object.
     * @param instPath Path to the instance data.
     * @returns        The constructed object.
     */
    static Params fromFile(Config const &config, std::string const &instPath);

    /**
     * Constructs a Params object with the given configuration, and passed-in
     * data. Assumes the data contains the depot, such that each vector is one
     * longer than the number of clients.
     *
     * @param config       Configuration object.
     * @param coords       Coordinates as pairs of [x, y].
     * @param demands      Client demands.
     * @param nbVehicles   Number of vehicles.
     * @param vehicleCap   Vehicle capacity.
     * @param timeWindows  Time windows as pairs of [early, late].
     * @param servDurs     Service durations.
     * @param distMat      Distance matrix.
     * @param releases     Client release times.
     */
    Params(Config const &config,
           std::vector<std::pair<int, int>> const &coords,
           std::vector<int> const &demands,
           int nbVehicles,
           int vehicleCap,
           std::vector<std::pair<int, int>> const &timeWindows,
           std::vector<int> const &servDurs,
           std::vector<std::vector<int>> const &distMat,
           std::vector<int> const &releases);
};

#endif
