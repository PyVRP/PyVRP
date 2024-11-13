#ifndef PYVRP_SOLUTION_H
#define PYVRP_SOLUTION_H

#include "Measure.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Route.h"

#include <functional>
#include <iosfwd>
#include <optional>
#include <vector>

namespace pyvrp
{
/**
 * Solution(data: ProblemData, routes: list[Route] | list[list[int]])
 *
 * Encodes VRP solutions.
 *
 * Parameters
 * ----------
 * data
 *     Data instance.
 * routes
 *     Route list to use. Can be a list of :class:`~Route` objects, or a lists
 *     of client visits. In case of the latter, all routes are assigned
 *     vehicles of the first type. That need not be a feasible assignment!
 *
 * Raises
 * ------
 * RuntimeError
 *     When the given solution is invalid in one of several ways. In
 *     particular when the number of routes in the ``routes`` argument exceeds
 *     :py:attr:`~ProblemData.num_vehicles`, when an empty route has been
 *     passed as part of ``routes``, when too many vehicles of a particular
 *     type have been used, or when a client is visited more than once.
 */
class Solution
{
    using Client = size_t;
    using Depot = size_t;
    using VehicleType = size_t;

    using Routes = std::vector<Route>;
    using Neighbours = std::vector<std::optional<std::pair<Client, Client>>>;

    size_t numClients_ = 0;         // Number of clients in the solution
    size_t numMissingClients_ = 0;  // Number of required but missing clients
    Distance distance_ = 0;         // Total travel distance over all routes
    Cost distanceCost_ = 0;         // Total cost of all routes' travel distance
    Duration duration_ = 0;         // Total duration over all routes
    Cost durationCost_ = 0;         // Total cost of all routes' duration
    Distance excessDistance_ = 0;   // Total excess distance over all routes
    std::vector<Load> excessLoad_;  // Total excess load over all routes
    Cost fixedVehicleCost_ = 0;     // Fixed cost of all used vehicles
    Cost prizes_ = 0;               // Total collected prize value
    Cost uncollectedPrizes_ = 0;    // Total uncollected prize value
    Duration timeWarp_ = 0;         // Total time warp over all routes
    bool isGroupFeas_ = true;       // Is feasible w.r.t. client groups?

    Routes routes_;
    Neighbours neighbours_;  // client [pred, succ] pairs, null if unassigned

    // Determines the [pred, succ] pairs for assigned clients.
    void makeNeighbours(ProblemData const &data);

    // Evaluates this solution's characteristics.
    void evaluate(ProblemData const &data);

    // These are only available within a solution; from the outside a solution
    // is immutable.
    Solution &operator=(Solution const &other) = default;
    Solution &operator=(Solution &&other) = default;

public:
    // Solution is empty when it has no routes and no clients.
    [[nodiscard]] bool empty() const;

    /**
     * Number of routes in this solution.
     */
    [[nodiscard]] size_t numRoutes() const;

    /**
     * Number of clients in this solution.
     *
     * .. warning::
     *
     *    An empty solution typically indicates that there is a significant
     *    difference between the values of the prizes of the optional clients
     *    and the other objective terms. This hints at a scaling issue in the
     *    data.
     */
    [[nodiscard]] size_t numClients() const;

    /**
     * Number of required clients that are not in this solution.
     */
    [[nodiscard]] size_t numMissingClients() const;

    /**
     * The solution's routing decisions.
     *
     * Returns
     * -------
     * list
     *     A list of routes. Each :class:`~Route` starts and ends at a depot,
     *     but that is implicit: the depot is not part of the returned routes.
     */
    [[nodiscard]] Routes const &routes() const;

    /**
     * Returns a list of neighbours for each client, by index.
     *
     * Returns
     * -------
     * list
     *     A list of ``(pred, succ)`` tuples that encode for each client their
     *     predecessor and successors in this solutions's routes. ``None`` in
     *     case the client is not in the solution (or is a depot).
     */
    [[nodiscard]] Neighbours const &neighbours() const;

    /**
     * Whether this solution is feasible.
     */
    [[nodiscard]] bool isFeasible() const;

    /**
     * Returns whether this solution is feasible w.r.t. the client group
     * restrictions.
     */
    [[nodiscard]] bool isGroupFeasible() const;

    /**
     * Returns whether this solution is complete, which it is when it has all
     * required clients.
     */
    [[nodiscard]] bool isComplete() const;

    /**
     * Returns whether this solution violates capacity constraints.
     */
    [[nodiscard]] bool hasExcessLoad() const;

    /**
     * Returns whether this solution violates maximum distance constraints.
     *
     * Returns
     * -------
     * bool
     *     True if the solution is not feasible with respect to the maximum
     *     distance constraints of the vehicles servicing routes in this
     *     solution. False otherwise.
     */
    [[nodiscard]] bool hasExcessDistance() const;

    /**
     * Returns whether this solution violates time window or maximum duration
     * constraints.
     */
    [[nodiscard]] bool hasTimeWarp() const;

    /**
     * Returns the total distance over all routes.
     */
    [[nodiscard]] Distance distance() const;

    /**
     * Total cost of the distance travelled on routes in this solution.
     */
    [[nodiscard]] Cost distanceCost() const;

    /**
     * Total duration of all routes in this solution.
     */
    [[nodiscard]] Duration duration() const;

    /**
     * Total cost of the duration of all routes in this solution.
     */
    [[nodiscard]] Cost durationCost() const;

    /**
     * Aggregate pickup or delivery loads in excess of the vehicle's capacity
     * of all routes.
     */
    [[nodiscard]] std::vector<Load> const &excessLoad() const;

    /**
     * Returns the total distance in excess of maximum duration constraints,
     * over all routes.
     */
    [[nodiscard]] Distance excessDistance() const;

    /**
     * Returns the fixed vehicle cost of all vehicles used in this solution.
     */
    [[nodiscard]] Cost fixedVehicleCost() const;

    /**
     * Returns the total collected prize value over all routes.
     */
    [[nodiscard]] Cost prizes() const;

    /**
     * Total prize value of all clients not visited in this solution.
     */
    [[nodiscard]] Cost uncollectedPrizes() const;

    /**
     * Returns the total time warp load over all routes.
     */
    [[nodiscard]] Duration timeWarp() const;

    bool operator==(Solution const &other) const;

    Solution(Solution const &other) = default;
    Solution(Solution &&other) = default;

    /**
     * make_random(data: ProblemData, rng: RandomNumberGenerator) -> Solution
     *
     * Creates a randomly generated solution.
     *
     * Parameters
     * ----------
     * data
     *     Data instance.
     * rng
     *     Random number generator to use.
     *
     * Returns
     * -------
     * Solution
     *     The randomly generated solution.
     */
    Solution(ProblemData const &data, RandomNumberGenerator &rng);

    // This constructs from the given lists of client indices. Assumes all
    // routes are intended to use vehicles of the first vehicle type.
    Solution(ProblemData const &data,
             std::vector<std::vector<Client>> const &routes);

    // This constructs from the given list of Routes.
    Solution(ProblemData const &data, Routes const &routes);

    // This constructor does *no* validation. Useful when unserialising objects.
    Solution(size_t numClients,
             size_t numMissingClients,
             Distance distance,
             Cost distanceCost,
             Duration duration,
             Cost durationCost,
             Distance excessDistance,
             std::vector<Load> excessLoad,
             Cost fixedVehicleCost,
             Cost prizes,
             Cost uncollectedPrizes,
             Duration timeWarp,
             bool isGroupFeasible,
             Routes routes,
             Neighbours neighbours);
};
}  // namespace pyvrp

std::ostream &operator<<(std::ostream &out, pyvrp::Solution const &sol);

template <> struct std::hash<pyvrp::Solution>
{
    size_t operator()(pyvrp::Solution const &sol) const
    {
        size_t res = 17;
        res = res * 31 + std::hash<size_t>()(sol.numRoutes());
        res = res * 31 + std::hash<pyvrp::Distance>()(sol.distance());
        res = res * 31 + std::hash<pyvrp::Duration>()(sol.duration());
        res = res * 31 + std::hash<pyvrp::Duration>()(sol.timeWarp());

        return res;
    }
};

#endif  // PYVRP_SOLUTION_H
