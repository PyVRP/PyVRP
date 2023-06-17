#ifndef PYVRP_INDIVIDUAL_H
#define PYVRP_INDIVIDUAL_H

#include "Measure.h"
#include "ProblemData.h"
#include "TimeWindowSegment.h"
#include "XorShift128.h"

#include <functional>
#include <iosfwd>
#include <vector>

class Individual
{
    friend struct std::hash<Individual>;  // friend struct to enable hashing

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
        Load demand_ = 0;        // Total demand served on this route
        Load excessLoad_ = 0;    // Excess demand (wrt vehicle capacity)
        Duration travel_ = 0;    // Total travel duration on this route
        Duration service_ = 0;   // Total service duration on this route
        TimeWindowSegment tws_;  // Time window segment of this route
        Cost prizes_ = 0;        // Total value of prizes on this route

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
        [[nodiscard]] Load demand() const;
        [[nodiscard]] Load excessLoad() const;
        [[nodiscard]] Duration travelDuration() const;
        [[nodiscard]] Duration serviceDuration() const;
        [[nodiscard]] Duration timeWarp() const;
        [[nodiscard]] Duration waitDuration() const;
        [[nodiscard]] Duration totalDuration() const;
        [[nodiscard]] Duration earliestStart() const;
        [[nodiscard]] Duration latestStart() const;
        [[nodiscard]] Duration slack() const;
        [[nodiscard]] Cost prizes() const;

        [[nodiscard]] std::pair<double, double> const &centroid() const;

        [[nodiscard]] bool isFeasible() const;
        [[nodiscard]] bool hasExcessLoad() const;
        [[nodiscard]] bool hasTimeWarp() const;

        Route() = default;  // default is empty
        Route(ProblemData const &data, Visits const visits);
    };

private:
    using Routes = std::vector<Route>;

    size_t numRoutes_ = 0;        // Number of routes
    size_t numClients_ = 0;       // Number of clients in the solution
    Distance distance_ = 0;       // Total distance
    Load excessLoad_ = 0;         // Total excess load over all routes
    Cost prizes_ = 0;             // Total collected prize value
    Cost uncollectedPrizes_ = 0;  // Total uncollected prize value
    Duration timeWarp_ = 0;       // Total time warp over all routes

    Routes routes_;  // Routes - only the first numRoutes_ are non-empty
    std::vector<std::pair<Client, Client>> neighbours;  // pairs of [pred, succ]

    // Determines the [pred, succ] pairs for each client.
    void makeNeighbours();

    // Evaluates this solution's characteristics.
    void evaluate(ProblemData const &data);

public:
    /**
     * Returns the number of non-empty routes in this individual's solution.
     * Such non-empty routes are guaranteed to be in the lower indices of the
     * routes returned by ``getRoutes``.
     */
    [[nodiscard]] size_t numRoutes() const;

    /**
     * Number of clients in the solution.
     */
    [[nodiscard]] size_t numClients() const;

    /**
     * Returns this individual's routing decisions.
     */
    [[nodiscard]] Routes const &getRoutes() const;

    /**
     * Returns a vector of [pred, succ] clients for each client (index) in this
     * individual's routes. Includes the depot at index 0.
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

    bool operator==(Individual const &other) const;

    Individual &operator=(Individual const &other) = delete;  // is immutable
    Individual &operator=(Individual &&other) = delete;       // is immutable

    Individual(Individual const &other) = default;
    Individual(Individual &&other) = default;

    /**
     * Constructs a random individual using the given random number generator.
     *
     * @param data           Data instance describing the problem that's being
     *                       solved.
     * @param rng            Random number generator.
     */
    Individual(ProblemData const &data, XorShift128 &rng);

    /**
     * Constructs an individual having the given routes as its solution.
     *
     * @param data           Data instance describing the problem that's being
     *                       solved.
     * @param routes         Solution's route list.
     */
    Individual(ProblemData const &data,
               std::vector<std::vector<Client>> const &routes);
};

std::ostream &operator<<(std::ostream &out, Individual const &indiv);
std::ostream &operator<<(std::ostream &out, Individual::Route const &route);

namespace std
{
template <> struct hash<Individual>
{
    size_t operator()(Individual const &individual) const
    {
        size_t res = 17;
        res = res * 31 + std::hash<size_t>()(individual.numRoutes_);
        res = res * 31 + std::hash<Distance>()(individual.distance_);
        res = res * 31 + std::hash<Load>()(individual.excessLoad_);
        res = res * 31 + std::hash<Duration>()(individual.timeWarp_);

        return res;
    }
};
}  // namespace std

#endif  // PYVRP_INDIVIDUAL_H
