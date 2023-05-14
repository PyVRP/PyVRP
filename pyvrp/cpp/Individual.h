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
     * A simple Route structure that contains the route plan (as a vector), and
     * some route statistics.
     */
    struct Route
    {
        using Plan = std::vector<Client>;

        Plan plan = {};       // Route plan (list of clients).
        size_t distance = 0;  // Total travel distance on this route
        size_t demand = 0;    // Total demand served on this route
        size_t duration = 0;  // Total travel duration on this route
        size_t service = 0;   // Total service duration on this route
        size_t timeWarp = 0;  // Total time warp on this route
        size_t wait = 0;      // Total waiting duration on this route

        bool empty() const { return plan.empty(); };
        size_t size() const { return plan.size(); };
        Client operator[](size_t idx) const { return plan[idx]; };

        void insert(auto idx, Client client) { plan.insert(idx, client); };
        void push_back(Client client) { plan.push_back(client); };

        auto begin() const { return plan.begin(); };
        auto end() const { return plan.end(); };
        auto back() const { return plan.back(); };

        Route() = default;  // default is empty
        Route(ProblemData const &data, Plan const &plan) : plan(plan)
        {
            if (plan.empty())
                return;

            int time = data.depot().twEarly;
            int prevClient = 0;

            for (size_t idx = 0; idx != size(); ++idx)
            {
                distance += data.dist(prevClient, plan[idx]);
                duration += data.duration(prevClient, plan[idx]);
                demand += data.client(plan[idx]).demand;

                time += data.client(prevClient).serviceDuration
                        + data.duration(prevClient, plan[idx]);

                if (time < data.client(plan[idx]).twEarly)  // add wait duration
                {
                    wait += data.client(plan[idx]).twEarly - time;
                    time = data.client(plan[idx]).twEarly;
                }

                if (time > data.client(plan[idx]).twLate)  // add time warp
                {
                    timeWarp += time - data.client(plan[idx]).twLate;
                    time = data.client(plan[idx]).twLate;
                }
            }

            // Last client has depot as successor.
            distance += data.dist(plan.back(), 0);
            duration += data.duration(plan.back(), 0);
            time += data.client(plan.back()).serviceDuration
                    + data.duration(plan.back(), 0);

            // For the depot we only need to check the end of the time window.
            timeWarp += std::max(time - data.depot().twLate, 0);
        };
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

// Outputs an individual into a given ostream in VRPLIB format
std::ostream &operator<<(std::ostream &out, Individual const &indiv);

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
