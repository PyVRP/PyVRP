#ifndef INDIVIDUAL_H
#define INDIVIDUAL_H

#include "ProblemData.h"
#include "XorShift128.h"

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
        size_t distance_ = 0;    // Total travel distance on this route
        size_t demand_ = 0;      // Total demand served on this route
        size_t excessLoad_ = 0;  // Demand in excess of the vehicle's capacity
        size_t duration_ = 0;    // Total travel duration on this route
        size_t service_ = 0;     // Total service duration on this route
        size_t timeWarp_ = 0;    // Total time warp on this route
        size_t wait_ = 0;        // Total waiting duration on this route
        size_t prizes_ = 0;      // Total prize value collected on this route

    public:
        [[nodiscard]] bool empty() const;
        [[nodiscard]] size_t size() const;
        [[nodiscard]] Client operator[](size_t idx) const;

        Visits::const_iterator begin() const;
        Visits::const_iterator end() const;
        Visits::const_iterator cbegin() const;
        Visits::const_iterator cend() const;

        [[nodiscard]] Visits const &visits() const;
        [[nodiscard]] size_t distance() const;
        [[nodiscard]] size_t demand() const;
        [[nodiscard]] size_t excessLoad() const;
        [[nodiscard]] size_t duration() const;
        [[nodiscard]] size_t serviceDuration() const;
        [[nodiscard]] size_t timeWarp() const;
        [[nodiscard]] size_t waitDuration() const;
        [[nodiscard]] size_t prizes() const;

        [[nodiscard]] bool isFeasible() const;
        [[nodiscard]] bool hasExcessLoad() const;
        [[nodiscard]] bool hasTimeWarp() const;

        Route() = default;  // default is empty
        Route(ProblemData const &data, Visits const visits);
    };

private:
    using Routes = std::vector<Route>;

    size_t numRoutes_ = 0;          // Number of routes
    size_t numClients_ = 0;         // Number of clients in the solution
    size_t distance_ = 0;           // Total distance
    size_t excessLoad_ = 0;         // Total excess load over all routes
    size_t prizes_ = 0;             // Total collected prize value
    size_t uncollectedPrizes_ = 0;  // Total uncollected prize value
    size_t timeWarp_ = 0;           // Total time warp over all routes

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
    [[nodiscard]] size_t distance() const;

    /**
     * @return Total excess load over all routes.
     */
    [[nodiscard]] size_t excessLoad() const;

    /**
     * @return Total collected prize value over all routes.
     */
    [[nodiscard]] size_t prizes() const;

    /**
     * @return Total prize value of all unvisited clients.
     */
    [[nodiscard]] size_t uncollectedPrizes() const;

    /**
     * @return Total time warp over all routes.
     */
    [[nodiscard]] size_t timeWarp() const;

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
    std::size_t operator()(Individual const &individual) const
    {
        size_t res = 17;
        res = res * 31 + std::hash<size_t>()(individual.numRoutes_);
        res = res * 31 + std::hash<size_t>()(individual.distance_);
        res = res * 31 + std::hash<size_t>()(individual.excessLoad_);
        res = res * 31 + std::hash<size_t>()(individual.timeWarp_);

        return res;
    }
};
}  // namespace std

#endif
