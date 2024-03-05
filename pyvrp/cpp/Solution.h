#ifndef PYVRP_SOLUTION_H
#define PYVRP_SOLUTION_H

#include "Measure.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"

#include <functional>
#include <iosfwd>
#include <optional>
#include <vector>

namespace pyvrp
{
/**
 * Solution(data: ProblemData, routes: Union[List[Route], List[List[int]]])
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

public:
    /**
     * Route(
     *     data: ProblemData,
     *     visits: List[int],
     *     vehicle_type: int,
     * )
     *
     * A simple class that stores the route plan and some statistics.
     */
    class Route
    {
        using Visits = std::vector<Client>;

        Visits visits_ = {};      // Client visits on this route
        Distance distance_ = 0;   // Total travel distance on this route
        Load delivery_ = 0;       // Total delivery amount served on this route
        Load pickup_ = 0;         // Total pickup amount gathered on this route
        Load excessLoad_ = 0;     // Excess pickup or delivery demand
        Duration duration_ = 0;   // Total duration of this route
        Duration timeWarp_ = 0;   // Total time warp on this route
        Duration travel_ = 0;     // Total *travel* duration on this route
        Duration service_ = 0;    // Total *service* duration on this route
        Duration wait_ = 0;       // Total *waiting* duration on this route
        Duration release_ = 0;    // Release time of this route
        Duration startTime_ = 0;  // (earliest) start time of this route
        Duration slack_ = 0;      // Total time slack on this route
        Cost prizes_ = 0;         // Total value of prizes on this route

        std::pair<double, double> centroid_;  // Route center
        VehicleType vehicleType_;             // Type of vehicle
        Depot depot_;                         // Assigned depot

    public:
        [[nodiscard]] bool empty() const;
        [[nodiscard]] size_t size() const;
        [[nodiscard]] Client operator[](size_t idx) const;

        Visits::const_iterator begin() const;
        Visits::const_iterator end() const;

        /**
         * Route visits, as a list of clients.
         */
        [[nodiscard]] Visits const &visits() const;

        /**
         * Total distance travelled on this route.
         */
        [[nodiscard]] Distance distance() const;

        /**
         * Total client delivery amount on this route.
         */
        [[nodiscard]] Load delivery() const;

        /**
         * Total client pickup amount on this route.
         */
        [[nodiscard]] Load pickup() const;

        /**
         * Pick or delivery demand in excess of the vehicle's capacity.
         */
        [[nodiscard]] Load excessLoad() const;

        /**
         * Total route duration, including travel, service and waiting time.
         */
        [[nodiscard]] Duration duration() const;

        /**
         * Total duration of service on this route.
         */
        [[nodiscard]] Duration serviceDuration() const;

        /**
         * Amount of time warp incurred on this route.
         */
        [[nodiscard]] Duration timeWarp() const;

        /**
         * Total duration of travel on this route.
         */
        [[nodiscard]] Duration travelDuration() const;

        /**
         * Total waiting duration on this route.
         */
        [[nodiscard]] Duration waitDuration() const;

        /**
         * Start time of this route. This is the earliest possible time at which
         * the route can leave the depot and have a minimal duration and time
         * warp. If there is positive :meth:`~slack`, the start time can be
         * delayed by at most :meth:`~slack` time units without increasing the
         * total (minimal) route duration, or time warp.
         *
         * .. note::
         *
         *    It may be possible to leave before the start time (if the depot
         *    time window allows for it). That will introduce additional waiting
         *    time, such that the route duration will then no longer be minimal.
         *    Delaying departure by more than :meth:`~slack` time units always
         *    increases time warp, which could turn the route infeasible.
         */
        [[nodiscard]] Duration startTime() const;

        /**
         * End time of the route. This is equivalent to
         * ``start_time + duration - time_warp``.
         */
        [[nodiscard]] Duration endTime() const;

        /**
         * Time by which departure from the depot can be delayed without
         * resulting in (additional) time warp or increased route duration.
         */
        [[nodiscard]] Duration slack() const;

        /**
         * Earliest time at which this route can leave the depot. Follows from
         * the release times of clients visited on this route.
         *
         * .. note::
         *
         *    The route's release time should not be later than its start time,
         *    unless the route has time warp.
         */
        [[nodiscard]] Duration releaseTime() const;

        /**
         * Total prize value collected on this route.
         */
        [[nodiscard]] Cost prizes() const;

        /**
         * Center point of the client locations on this route.
         */
        [[nodiscard]] std::pair<double, double> const &centroid() const;

        /**
         * Index of the type of vehicle used on this route.
         */
        [[nodiscard]] VehicleType vehicleType() const;

        /**
         * Location index of the route's depot.
         */
        [[nodiscard]] Depot depot() const;

        [[nodiscard]] bool isFeasible() const;
        [[nodiscard]] bool hasExcessLoad() const;
        [[nodiscard]] bool hasTimeWarp() const;

        bool operator==(Route const &other) const;

        Route() = delete;

        Route(ProblemData const &data,
              Visits visits,
              VehicleType const vehicleType);

        // This constructor does *no* validation. Useful when unserialising
        // objects.
        Route(Visits visits,
              Distance distance,
              Load delivery,
              Load pickup,
              Load excessLoad,
              Duration duration,
              Duration timeWarp,
              Duration travel,
              Duration service,
              Duration wait,
              Duration release,
              Duration startTime,
              Duration slack,
              Cost prizes,
              std::pair<double, double> centroid,
              VehicleType vehicleType,
              Depot depot);
    };

private:
    using Routes = std::vector<Route>;
    using Neighbours = std::vector<std::optional<std::pair<Client, Client>>>;

    size_t numClients_ = 0;         // Number of clients in the solution
    size_t numMissingClients_ = 0;  // Number of required but missing clients
    Distance distance_ = 0;         // Total distance
    Load excessLoad_ = 0;           // Total excess load over all routes
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
     *
     * Returns
     * -------
     * int
     *     Number of routes.
     */
    [[nodiscard]] size_t numRoutes() const;

    /**
     * Number of clients in this solution.
     *
     * Returns
     * -------
     * int
     *     Number of clients in this solution.
     */
    [[nodiscard]] size_t numClients() const;

    /**
     * Number of required clients that are not in this solution.
     *
     * Returns
     * -------
     * int
     *     Number of required but missing clients.
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
     * Returns whether this solution is feasible w.r.t. the mutually exclusive
     * client groups: of each group, exactly one client must be in the solution.
     */
    [[nodiscard]] bool isGroupFeasible() const;

    /**
     * Returns whether this solution is complete, which it is when it has all
     * required clients.
     *
     * Returns
     * -------
     * bool
     *     True if the solution visits all required clients, False otherwise.
     */
    [[nodiscard]] bool isComplete() const;

    /**
     * Returns whether this solution violates capacity constraints.
     *
     * Returns
     * -------
     * bool
     *     True if the solution is not capacity feasible, False otherwise.
     */
    [[nodiscard]] bool hasExcessLoad() const;

    /**
     * Returns whether this solution violates time window constraints.
     *
     * Returns
     * -------
     * bool
     *     True if the solution is not time window feasible, False
     *     otherwise.
     */
    [[nodiscard]] bool hasTimeWarp() const;

    /**
     * Returns the total distance over all routes.
     *
     * Returns
     * -------
     * int
     *     Total distance over all routes.
     */
    [[nodiscard]] Distance distance() const;

    /**
     * Returns the total excess load over all routes.
     *
     * Returns
     * -------
     * int
     *     Total excess load over all routes.
     */
    [[nodiscard]] Load excessLoad() const;

    /**
     * Returns the fixed vehicle cost of all vehicles used in this solution.
     *
     * Returns
     * -------
     * int
     *     Total fixed vehicle cost.
     */
    [[nodiscard]] Cost fixedVehicleCost() const;

    /**
     * Returns the total collected prize value over all routes.
     *
     * Returns
     * -------
     * int
     *     Value of collected prizes.
     */
    [[nodiscard]] Cost prizes() const;

    /**
     * Total prize value of all clients not visited in this solution.
     *
     * Returns
     * -------
     * int
     *     Value of uncollected prizes.
     */
    [[nodiscard]] Cost uncollectedPrizes() const;

    /**
     * Returns the total time warp load over all routes.
     *
     * Returns
     * -------
     * int
     *     Total time warp over all routes.
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
    Solution(ProblemData const &data, Routes const &routes);

    // This constructor does *no* validation. Useful when unserialising objects.
    Solution(size_t numClients,
             size_t numMissingClients,
             Distance distance,
             Load excessLoad,
             Cost fixedVehicleCost,
             Cost prizes,
             Cost uncollectedPrizes,
             Duration timeWarp,
             bool isGroupFeasible,
             Routes const &routes,
             Neighbours neighbours);
};
}  // namespace pyvrp

std::ostream &operator<<(std::ostream &out, pyvrp::Solution const &sol);
std::ostream &operator<<(std::ostream &out,
                         pyvrp::Solution::Route const &route);

template <> struct std::hash<pyvrp::Solution>
{
    size_t operator()(pyvrp::Solution const &sol) const
    {
        size_t res = 17;
        res = res * 31 + std::hash<size_t>()(sol.numRoutes());
        res = res * 31 + std::hash<pyvrp::Distance>()(sol.distance());
        res = res * 31 + std::hash<pyvrp::Load>()(sol.excessLoad());
        res = res * 31 + std::hash<pyvrp::Duration>()(sol.timeWarp());

        return res;
    }
};

#endif  // PYVRP_SOLUTION_H
