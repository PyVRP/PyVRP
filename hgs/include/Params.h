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

    // Penalty booster that increases the penalty on capacity and time window
    // violations during the object's lifetime.
    struct PenaltyBooster
    {
        Params *d_params;

        explicit PenaltyBooster(Params *params) : d_params(params)
        {
            d_params->penaltyCapacity *= d_params->config.repairBooster;
            d_params->penaltyTimeWarp *= d_params->config.repairBooster;
        }

        ~PenaltyBooster()
        {
            d_params->penaltyCapacity /= d_params->config.repairBooster;
            d_params->penaltyTimeWarp /= d_params->config.repairBooster;
        }
    };

    // Neighborhood restrictions: For each client, list of nearby clients (size
    // nbClients + 1, but nothing stored for the depot!)
    std::vector<std::vector<int>> neighbours;

    Matrix<int> dist_;  // Distance matrix (+depot)
    int maxDist_;       // Maximum distance in the distance matrix

    /**
     * Calculate, for all vertices, the correlation ('nearness') of the
     * nbGranular closest vertices.
     */
    void calculateNeighbours();

public:
    // TODO make members private

    Config const config;  // Stores all the parameter values

    int penaltyCapacity;  // Excess capacity penalty (per unit)
    int penaltyTimeWarp;  // Time warp penalty (per unit)

    int nbClients;        // Number of clients (excluding the depot)
    int nbVehicles;       // Number of vehicles
    int vehicleCapacity;  // Capacity limit

    std::vector<Client> clients;  // Client (+depot) information

    /**
     * Computes the total excess capacity penalty for the given load.
     */
    [[nodiscard]] int loadPenalty(int load) const
    {
        return std::max(load - vehicleCapacity, 0) * penaltyCapacity;
    }

    /**
     * Computes the total time warp penalty for the give time warp.
     */
    [[nodiscard]] int twPenalty(int timeWarp) const
    {
        return timeWarp * penaltyTimeWarp;
    }

    /**
     * Returns a penalty booster that temporarily increases infeasibility
     * penalties (while the booster lives).
     */
    [[nodiscard]] PenaltyBooster getPenaltyBooster()
    {
        return PenaltyBooster(this);
    }

    /**
     * Returns the nbGranular clients nearest/closest to the passed-in client.
     */
    [[nodiscard]] std::vector<int> const &getNeighboursOf(size_t client) const
    {
        return neighbours[client];
    }

    [[nodiscard]] int maxDist() const { return maxDist_; }

    [[nodiscard]] int &dist(size_t row, size_t col) { return dist_(row, col); }

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
     * Constructs a Params object with the given configuration, and data read
     * from the given instance path.
     *
     * @param config   Configuration object.
     * @param instPath Path to the instance data.
     */
    Params(Config const &config, std::string const &instPath);

    /**
     * Constructs a Params object with the given configuration, and passed-in
     * data. Assumes the data contains the depot, such that each vector is one
     * longer than the number of clients.
     *
     * @param config       Configuration object.
     * @param coords       Coordinates as pairs of [x, y].
     * @param demands      Client demands.
     * @param vehicleCap   Vehicle capacity.
     * @param timeWindows  Time windows as pairs of [early, late].
     * @param servDurs     Service durations.
     * @param distMat      Distance matrix.
     * @param releases     Client release times.
     */
    Params(Config const &config,
           std::vector<std::pair<int, int>> const &coords,
           std::vector<int> const &demands,
           int vehicleCap,
           std::vector<std::pair<int, int>> const &timeWindows,
           std::vector<int> const &servDurs,
           std::vector<std::vector<int>> const &distMat,
           std::vector<int> const &releases);
};

#endif
