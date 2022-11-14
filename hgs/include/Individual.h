#ifndef INDIVIDUAL_H
#define INDIVIDUAL_H

#include "Params.h"
#include "XorShift128.h"

#include <string>
#include <vector>

// Object to represent one individual of a population.
class Individual
{
    using Client = int;
    using Route = std::vector<Client>;
    using Routes = std::vector<Route>;

    size_t nbRoutes = 0;        // Number of routes
    size_t distance = 0;        // Total distance
    size_t capacityExcess = 0;  // Total excess load over all routes
    size_t timeWarp = 0;        // All route time warp of late arrivals

    // The other individuals in the population (cannot be the depot 0), ordered
    // by increasing proximity.
    std::vector<std::pair<int, Individual *>> indivsByProximity;

    Params const *params;  // Problem parameters

    // For each vehicle, the associated sequence of deliveries (complete
    // solution). Size is nbVehicles, but quite a few routes are likely empty
    // - the numRoutes() member indicates the number of nonempty routes.
    Routes routes_;

    // Pairs of [predecessor, successor] for each client (index)
    std::vector<std::pair<Client, Client>> neighbours;

    // Determines (pred, succ) pairs for each client
    void makeNeighbours();

    // Evaluates this solution's objective value.
    void evaluateCompleteCost();

public:
    /**
     * Returns this individual's objective (penalized cost).
     */
    [[nodiscard]] size_t cost() const
    {
        // clang-format off
        return distance
             + params->loadPenalty(params->vehicleCapacity + capacityExcess)
             + params->twPenalty(timeWarp);
        // clang-format on
    }

    /**
     * Returns the number of non-empty routes in this individual's solution.
     * Such non-empty routes are all in the lower indices (guarantee) of the
     * routes returned by ``getRoutes``.
     */
    [[nodiscard]] size_t numRoutes() const { return nbRoutes; }

    /**
     * Returns this individual's routing decisions.
     */
    [[nodiscard]] Routes const &getRoutes() const { return routes_; }

    /**
     * Returns a vector of [pred, succ] clients for each client (index) in this
     * individual's routes.
     */
    [[nodiscard]] std::vector<std::pair<Client, Client>> const &
    getNeighbours() const
    {
        return neighbours;
    }

    /**
     * Returns true when this solution is feasible; false otherwise.
     */
    [[nodiscard]] bool isFeasible() const
    {
        return !hasExcessCapacity() && !hasTimeWarp();
    }

    /**
     * If true, then the route exceeds vehicle capacity.
     */
    [[nodiscard]] bool hasExcessCapacity() const { return capacityExcess > 0; }

    /**
     * If true, then the route violates time window constraints.
     */
    [[nodiscard]] bool hasTimeWarp() const { return timeWarp > 0; }

    /**
     * Returns true when there exists another, identical individual.
     */
    [[nodiscard]] bool hasClone() const
    {
        return !indivsByProximity.empty()
               && indivsByProximity.begin()->first == 0;
    }

    // Computes and returns a distance measure with another individual, based
    // on the number of arcs that differ between two solutions.
    int brokenPairsDistance(Individual const *other) const;

    // Updates the proximity structures of this and the other individual.
    void registerNearbyIndividual(Individual *other);

    // Returns the average distance of this individual to the individuals
    // nearest to it.
    [[nodiscard]] double avgBrokenPairsDistanceClosest() const;

    // Exports a solution in CVRPLib format (adds a final line with the
    // computational time).
    void exportCVRPLibFormat(std::string const &path, double time) const;

    bool operator<(Individual const &other) const
    {
        return cost() < other.cost();
    }

    bool operator==(Individual const &other) const
    {
        return cost() == other.cost() && routes_ == other.routes_;
    }

    Individual &operator=(Individual const &other) = default;

    Individual(Params const *params, XorShift128 *rng);  // random individual

    Individual(Params const *params, Routes routes);

    Individual(Individual const &other);  // copy from other

    ~Individual();
};

// Outputs an individual into a given ostream in CVRPLib format
std::ostream &operator<<(std::ostream &out, Individual const &indiv);

#endif
