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

public:
    /**
     * A simple Route class that contains the route plan and some statistics.
     */
    class Route
    {
        using Visits = std::vector<Client>;

        Visits visits_ = {};     // Client visits on this route
        Distance distance_ = 0;  // Total travel distance on this route
        Load demandWeight_ = 0;        // Total weight demand served on this route
        Load demandVolume_ = 0;        // Total volume demand served on this route
        Salvage demandSalvage_ = 0;    // Total number of nonterminal salvage stops on this route
        Load excessWeight_ = 0;    // Excess weight demand (wrt vehicle weight capacity)
        Load excessVolume_ = 0;    // Excess volume demand (wrt vehicle volume capacity)
        Salvage excessSalvage_ = 0; // Number of excess salvage stops on this route above max (0)
        Salvage excessSalvageSequence_ = 0; // Number of excess salvage stops on this route above max (0)
        Duration duration_ = 0;  // Total travel duration on this route
        Duration service_ = 0;   // Total service duration on this route
        Duration timeWarp_ = 0;  // Total time warp on this route
        Duration wait_ = 0;      // Total waiting duration on this route
        Cost prizes_ = 0;        // Total value of prizes on this route
        bool salvageBeforeDelivery_ = false; // Apart from excess salvage, salvage must be sequenced properly

        std::pair<double, double> centroid_;  // center of the route

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
        [[nodiscard]] Load demandWeight() const;
        [[nodiscard]] Load demandVolume() const;
        [[nodiscard]] Salvage demandSalvage() const;
        [[nodiscard]] Load excessWeight() const;
        [[nodiscard]] Load excessVolume() const;
        [[nodiscard]] Salvage excessSalvage() const;
        [[nodiscard]] Salvage excessSalvageSequence() const;
        [[nodiscard]] Duration duration() const;
        [[nodiscard]] Duration serviceDuration() const;
        [[nodiscard]] Duration timeWarp() const;
        [[nodiscard]] Duration waitDuration() const;
        [[nodiscard]] Cost prizes() const;

        [[nodiscard]] std::pair<double, double> const &centroid() const;

        [[nodiscard]] bool isFeasible() const;
        [[nodiscard]] bool hasExcessWeight() const;
        [[nodiscard]] bool hasExcessVolume() const;
        [[nodiscard]] bool hasExcessSalvage() const;
        [[nodiscard]] bool hasExcessSalvageSequence() const;
        [[nodiscard]] bool hasSalvageBeforeDelivery() const;
        [[nodiscard]] bool hasTimeWarp() const;

        Route() = default;  // default is empty
        Route(ProblemData const &data, Visits const visits);
    };

private:
    using Routes = std::vector<Route>;

    size_t numClients_ = 0;       // Number of clients in the solution
    Distance distance_ = 0;       // Total distance
    Load excessWeight_ = 0;         // Total excess weight load over all routes
    Load excessVolume_ = 0;         // Total excess volume load over all routes
    Salvage excessSalvage_ = 0; // Total excess salvage stop over all routes
    Salvage excessSalvageSequence_ = 0; // Total excess salvage stop over all routes
    bool salvageBeforeDelivery_ = false; // Does the route contain a salvage before delivery
    Cost prizes_ = 0;             // Total collected prize value
    Cost uncollectedPrizes_ = 0;  // Total uncollected prize value
    Duration timeWarp_ = 0;       // Total time warp over all routes

    Routes routes_;  // Routes - only includes non-empty routes
    std::vector<std::pair<Client, Client>> neighbours;  // pairs of [pred, succ]

    // Determines the [pred, succ] pairs for each client.
    void makeNeighbours();

    // Evaluates this solution's characteristics.
    void evaluate(ProblemData const &data);

public:
    /**
     * Returns the number of (non-empty) routes in this solution. Equal to the
     * length of the vector of routes returned by ``getRoutes``.
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
     * @return True if the solution violates weight constraints.
     */
    [[nodiscard]] bool hasExcessWeight() const;

    /**
     * @return True if the solution violates volume constraints.
     */
    [[nodiscard]] bool hasExcessVolume() const;

    /**
     * @return True if the solution violates salvage constraints.
     */
    [[nodiscard]] bool hasExcessSalvage() const;

    /**
     * @return True if the solution violates salvage constraints.
     */
    [[nodiscard]] bool hasExcessSalvageSequence() const;

    /**
     * @return True if the solution sequence violates salvage constraints.
     */
    [[nodiscard]] bool hasSalvageBeforeDelivery() const;

    /**
     * @return True if the solution violates time window constraints.
     */
    [[nodiscard]] bool hasTimeWarp() const;

    /**
     * @return Total distance over all routes.
     */
    [[nodiscard]] Distance distance() const;

    /**
     * @return Total excess load weight over all routes.
     */
    [[nodiscard]] Load excessWeight() const;

    /**
     * @return Total excess salvage stops over all routes.
     */
    [[nodiscard]] Salvage excessSalvage() const;

    /**
     * @return Total excess salvage stops over all routes.
     */
    [[nodiscard]] Salvage excessSalvageSequence() const;

    /**
     * @return Total excess load volume over all routes.
     */
    [[nodiscard]] Load excessVolume() const;

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

    Solution &operator=(Solution const &other) = delete;  // is immutable
    Solution &operator=(Solution &&other) = delete;       // is immutable

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
     * Constructs a solution with the given routes.
     *
     * @param data   Data instance describing the problem that's being solved.
     * @param routes Solution's route list.
     */
    Solution(ProblemData const &data,
             std::vector<std::vector<Client>> const &routes);
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
        res = res * 31 + std::hash<Load>()(sol.excessWeight_);
        res = res * 31 + std::hash<Load>()(sol.excessVolume_);
        res = res * 31 + std::hash<Salvage>()(sol.excessSalvage_);
        res = res * 31 + std::hash<Salvage>()(sol.excessSalvageSequence_);
        res = res * 31 + std::hash<Duration>()(sol.timeWarp_);

        return res;
    }
};
}  // namespace std

#endif  // PYVRP_SOLUTION_H
