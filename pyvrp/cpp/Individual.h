#ifndef INDIVIDUAL_H
#define INDIVIDUAL_H

#include "PenaltyManager.h"
#include "ProblemData.h"
#include "XorShift128.h"

#include <string>
#include <vector>

class Individual
{
    using Client = int;
    using Route = std::vector<Client>;
    using Routes = std::vector<Route>;

    size_t nbRoutes = 0;        // Number of routes
    size_t distance = 0;        // Total distance
    size_t capacityExcess = 0;  // Total excess load over all routes
    size_t timeWarp = 0;        // All route time warp of late arrivals

    ProblemData const *data;
    PenaltyManager const *penaltyManager;

    Routes routes_;  // Routes - only the first nbRoutes are non-empty
    std::vector<std::pair<Client, Client>> neighbours;  // pairs of [pred, succ]

    // Determines the [pred, succ] pairs for each client.
    void makeNeighbours();

    // Evaluates this solution's objective value.
    void evaluateCompleteCost();

public:
    /**
     * Returns this individual's objective (penalized cost).
     */
    [[nodiscard]] size_t cost() const;

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
     * individual's routes.
     */
    [[nodiscard]] std::vector<std::pair<Client, Client>> const &
    getNeighbours() const;

    /**
     * @return True when this solution is feasible; false otherwise.
     */
    [[nodiscard]] inline bool isFeasible() const;

    /**
     * @return True if the solution violates load constraints.
     */
    [[nodiscard]] inline bool hasExcessCapacity() const;

    /**
     * @return True if the solution violates time window constraints.
     */
    [[nodiscard]] inline bool hasTimeWarp() const;

    bool operator==(Individual const &other) const;

    Individual &operator=(Individual const &other) = default;

    /**
     * Constructs a random individual using the given random number generator.
     *
     * @param data           Data instance describing the problem that's being
     *                       solved.
     * @param penaltyManager Penalty manager, used to compute the objective.
     * @param rng            Random number generator.
     */
    Individual(ProblemData const &data,
               PenaltyManager const &penaltyManager,
               XorShift128 &rng);

    /**
     * Constructs an individual having the given routes as its solution.
     *
     * @param data           Data instance describing the problem that's being
     *                       solved.
     * @param penaltyManager Penalty manager, used to compute the objective.
     * @param routes         Solution's route list.
     */
    Individual(ProblemData const &data,
               PenaltyManager const &penaltyManager,
               Routes routes);

    /**
     * Constructs a copy of the given other individual.
     *
     * @param other Other individual to copy.
     */
    Individual(Individual const &other);
};

// Outputs an individual into a given ostream in VRPLIB format
std::ostream &operator<<(std::ostream &out, Individual const &indiv);

bool Individual::isFeasible() const
{
    return !hasExcessCapacity() && !hasTimeWarp();
}

bool Individual::hasExcessCapacity() const { return capacityExcess > 0; }

bool Individual::hasTimeWarp() const 
{ 
#ifdef VRP_NO_TIME_WINDOWS
    return false;
#else
    return timeWarp > 0;
#endif
}

#endif
