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
     * A simple Route class that contains the route plan (as a vector), and
     * some route statistics.
     */
    class Route
    {
        using Plan = std::vector<Client>;

        Plan plan_ = {};         // Route plan (list of clients).
        size_t distance_ = 0;    // Total travel distance on this route
        size_t demand_ = 0;      // Total demand served on this route
        size_t excessLoad_ = 0;  // Demand in excess of the vehicle's capacity
        size_t duration_ = 0;    // Total travel duration on this route
        size_t service_ = 0;     // Total service duration on this route
        size_t timeWarp_ = 0;    // Total time warp on this route
        size_t wait_ = 0;        // Total waiting duration on this route

    public:
        bool empty() const;
        size_t size() const;
        Client operator[](size_t idx) const;

        Plan::const_iterator begin() const;
        Plan::const_iterator end() const;
        Plan::const_iterator cbegin() const;
        Plan::const_iterator cend() const;

        Plan const &plan() const;
        size_t distance() const;
        size_t demand() const;
        size_t excessLoad() const;
        size_t duration() const;
        size_t service() const;
        size_t timeWarp() const;
        size_t wait() const;

        bool isFeasible() const;
        bool hasExcessLoad() const;
        bool hasTimeWarp() const;

        Route() = default;  // default is empty
        Route(ProblemData const &data, Plan const &plan);
    };

private:
    using Routes = std::vector<Route>;

    size_t numRoutes_ = 0;   // Number of routes
    size_t distance_ = 0;    // Total distance
    size_t excessLoad_ = 0;  // Total excess load over all routes
    size_t timeWarp_ = 0;    // Total time warp over all routes

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
               std::vector<std::vector<Client>> routes);

    /**
     * Constructs an individual having the given routes as its solution.
     *
     * @param data           Data instance describing the problem that's being
     *                       solved.
     * @param routes         Solution's route list.
     */
    Individual(ProblemData const &data, Routes routes);
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
