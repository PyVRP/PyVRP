#ifndef PYVRP_SOLUTION_H
#define PYVRP_SOLUTION_H

#include "Measure.h"
#include "ProblemData.h"
#include "XorShift128.h"

#include <functional>
#include <iosfwd>
#include <vector>

class Solution
{
    friend struct std::hash<Solution>;  // friend struct to enable hashing

    using Client = int;
    using VehicleType = size_t;

public:
    /**
     * A simple Route class that contains the route plan and some statistics.
     */
    class Route
    {
        using Visits = std::vector<Client>;

        Visits visits_ = {};     // Client visits on this route
        Distance distance_ = 0;  // Total travel distance on this route
        Load demand_ = 0;        // Total demand served on this route
        Load excessLoad_ = 0;    // Excess demand (wrt vehicle capacity)
        Duration duration_ = 0;  // Total travel duration on this route
        Duration service_ = 0;   // Total service duration on this route
        Duration timeWarp_ = 0;  // Total time warp on this route
        Duration wait_ = 0;      // Total waiting duration on this route
        Duration release_ = 0;   // Release time of this route
        Cost prizes_ = 0;        // Total value of prizes on this route

        std::pair<double, double> centroid_;  // center of the route
        VehicleType vehicleType_ = 0;         // Type of vehicle of this route

    public:
        [[nodiscard]] bool empty() const;
        [[nodiscard]] size_t size() const;
        [[nodiscard]] Client operator[](size_t idx) const;

        Visits::const_iterator begin() const;
        Visits::const_iterator end() const;
        Visits::const_iterator cbegin() const;
        Visits::const_iterator cend() const;

        [[nodiscard]] Visits const &visits() const;
        [[nodiscard]] Distance distance() const;
        [[nodiscard]] Load demand() const;
        [[nodiscard]] Load excessLoad() const;
        [[nodiscard]] Duration duration() const;
        [[nodiscard]] Duration serviceDuration() const;
        [[nodiscard]] Duration timeWarp() const;
        [[nodiscard]] Duration waitDuration() const;
        [[nodiscard]] Duration releaseTime() const;
        [[nodiscard]] Cost prizes() const;

        [[nodiscard]] std::pair<double, double> const &centroid() const;
        [[nodiscard]] VehicleType vehicleType() const;

        [[nodiscard]] bool isFeasible() const;
        [[nodiscard]] bool hasExcessLoad() const;
        [[nodiscard]] bool hasTimeWarp() const;

        bool operator==(Route const &other) const;

        Route() = default;  // default is empty
        Route(ProblemData const &data,
              Visits const visits,
              VehicleType const vehicleType);
    };

private:
    using Routes = std::vector<Route>;

    size_t numClients_ = 0;       // Number of clients in the solution
    Distance distance_ = 0;       // Total distance
    Load excessLoad_ = 0;         // Total excess load over all routes
    Cost prizes_ = 0;             // Total collected prize value
    Cost uncollectedPrizes_ = 0;  // Total uncollected prize value
    Duration timeWarp_ = 0;       // Total time warp over all routes

    Routes routes_;
    std::vector<std::pair<Client, Client>> neighbours;  // pairs of [pred, succ]

    // Determines the [pred, succ] pairs for each client.
    void makeNeighbours();

    // Evaluates this solution's characteristics.
    void evaluate(ProblemData const &data);

    // These are only available within a solution; from the outside a solution
    // is immutable.
    Solution &operator=(Solution const &other) = default;
    Solution &operator=(Solution &&other) = default;

public:
    /**
     * Returns the number of routes in this solution. Equal to the length of
     * the vector of routes returned by ``getRoutes``.
     */
    [[nodiscard]] size_t numRoutes() const;

    /**
     * Number of clients in the solution.
     */
    [[nodiscard]] size_t numClients() const;

    /**
     * Returns the routing decisions.
     */
    [[nodiscard]] Routes const &getRoutes() const;

    /**
     * Returns a vector of [pred, succ] clients for each client (index) in this
     * solutions's routes. Includes the depot at index 0.
     */
    [[nodiscard]] std::vector<std::pair<Client, Client>> const &
    getNeighbours() const;

    /**
     * @return True when this solution is feasible; false otherwise.
     */
    [[nodiscard]] bool isFeasible() const;

    /**
     * @return True if the solution violates load constraints.
     */
    [[nodiscard]] bool hasExcessLoad() const;

    /**
     * @return True if the solution violates time window constraints.
     */
    [[nodiscard]] bool hasTimeWarp() const;

    /**
     * @return Total distance over all routes.
     */
    [[nodiscard]] Distance distance() const;

    /**
     * @return Total excess load over all routes.
     */
    [[nodiscard]] Load excessLoad() const;

    /**
     * @return Total collected prize value over all routes.
     */
    [[nodiscard]] Cost prizes() const;

    /**
     * @return Total prize value of all unvisited clients.
     */
    [[nodiscard]] Cost uncollectedPrizes() const;

    /**
     * @return Total time warp over all routes.
     */
    [[nodiscard]] Duration timeWarp() const;

    bool operator==(Solution const &other) const;

    Solution(Solution const &other) = default;
    Solution(Solution &&other) = default;

    /**
     * Constructs a random solution using the given random number generator.
     *
     * @param data Data instance describing the problem that's being solved.
     * @param rng  Random number generator.
     */
    Solution(ProblemData const &data, XorShift128 &rng);

    /**
     * Constructs a solution using routes given as lists of client indices.
     * This constructor assumes all routes use vehicles having vehicle type 0.
     *
     * @param data   Data instance describing the problem that's being solved.
     * @param routes Solution's route list.
     */
    Solution(ProblemData const &data,
             std::vector<std::vector<Client>> const &routes);

    /**
     * Constructs a solution from the given list of Routes.
     *
     * @param data   Data instance describing the problem that's being solved.
     * @param routes Solution's route list.
     */
    Solution(ProblemData const &data, std::vector<Route> const &routes);
};

std::ostream &operator<<(std::ostream &out, Solution const &sol);
std::ostream &operator<<(std::ostream &out, Solution::Route const &route);

namespace std
{
template <> struct hash<Solution>
{
    size_t operator()(Solution const &sol) const
    {
        size_t res = 17;
        res = res * 31 + std::hash<size_t>()(sol.routes_.size());
        res = res * 31 + std::hash<Distance>()(sol.distance_);
        res = res * 31 + std::hash<Load>()(sol.excessLoad_);
        res = res * 31 + std::hash<Duration>()(sol.timeWarp_);

        return res;
    }
};
}  // namespace std

#endif  // PYVRP_SOLUTION_H
