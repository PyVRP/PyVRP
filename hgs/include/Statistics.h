#ifndef STATISTICS_H
#define STATISTICS_H

#include "Population.h"

#include <chrono>
#include <vector>

class Population;  // forward declaration

class Statistics
{
    using clock = std::chrono::system_clock;
    using timedDatapoints = std::vector<std::pair<double, size_t>>;

    clock::time_point start = clock::now();
    clock::time_point lastIter = clock::now();
    size_t numIters_ = 0;

    std::vector<double> runTimes_;
    std::vector<double> iterTimes_;

public:
    struct SubPopStats
    {
        std::vector<size_t> popSize_;
        std::vector<double> avgDiversity_;
        std::vector<size_t> bestCost_;
        std::vector<size_t> avgCost_;
        std::vector<double> avgNumRoutes_;
    };

private:
    SubPopStats feasStats;
    SubPopStats infeasStats;

    std::vector<size_t> penaltiesCapacity_;
    std::vector<size_t> penaltiesTimeWarp_;

    timedDatapoints incumbents_;

public:
    /**
     * Collects population and objective value statistics. This function is
     * called repeatedly during the genetic algorithm's search, and stores
     * relevant data for later evaluation.
     *
     * @param population  Population object to collect data from.
     */
    void collectFrom(Population const &population);

    /**
     * Returns the total number of iterations.
     */
    [[nodiscard]] size_t numIters() const { return numIters_; }

    /**
     * Returns a vector of run times in seconds, one element per iteration.
     * Each element indicates the time between the current iteration and the
     * start of the algorithm.
     */
    [[nodiscard]] std::vector<double> const &runTimes() const
    {
        return runTimes_;
    }

    /**
     * Returns a vector of run times in seconds, one element per iteration.
     * Each element indicates the time between the current and previous
     * iteration.
     */
    [[nodiscard]] std::vector<double> const &iterTimes() const
    {
        return iterTimes_;
    }

    /**
     * Returns a vector of the number of feasible individuals in the population,
     * one element per iteration.
     */
    [[nodiscard]] std::vector<size_t> const &feasPopSize() const
    {
        return feasStats.popSize_;
    }

    /**
     * Returns a vector of the average feasible sub-population diversity, one
     * element per iteration. The average diversity is computed as the average
     * broken pairs distance for each individual in the sub-population, compared
     * to its neighbours (the neighbourhood size is controlled by the
     * ``nbClose`` setting).
     */
    [[nodiscard]] std::vector<double> const &feasAvgDiversity() const
    {
        return feasStats.avgDiversity_;
    }

    /**
     * Returns a vector of the best objective value of feasible individuals,
     * one element per iteration. If there are no feasible individuals, then
     * ``INT_MAX`` is stored.
     */
    [[nodiscard]] std::vector<size_t> const &feasBestCost() const
    {
        return feasStats.bestCost_;
    }

    /**
     * Returns a vector of the average objective value of feasible individuals,
     * one element per iteration. If there are no feasible individuals, then
     * ``INT_MAX`` is stored.
     */
    [[nodiscard]] std::vector<size_t> const &feasAvgCost() const
    {
        return feasStats.avgCost_;
    }

    /**
     * Returns a vector of the average number of routes among feasible
     * individuals, one element per iteration. If there are no feasible
     * individuals, then 0 is stored.
     */
    [[nodiscard]] std::vector<double> const &feasAvgNumRoutes() const
    {
        return feasStats.avgNumRoutes_;
    }

    /**
     * Returns a vector of the number of infeasible individuals in the
     * population, one element per iteration.
     */
    [[nodiscard]] std::vector<size_t> const &infeasPopSize() const
    {
        return infeasStats.popSize_;
    }

    /**
     * Returns a vector of the average infeasible sub-population diversity, one
     * element per iteration. The average diversity is computed as the average
     * broken pairs distance for each individual in the sub-population, compared
     * to its neighbours (the neighbourhood size is controlled by the
     * ``nbClose`` setting).
     */
    [[nodiscard]] std::vector<double> const &infeasAvgDiversity() const
    {
        return infeasStats.avgDiversity_;
    }

    /**
     * Returns a vector of the best objective value of infeasible individuals,
     * one element per iteration. If there are no infeasible individuals, then
     * ``INT_MAX`` is stored.
     */
    [[nodiscard]] std::vector<size_t> const &infeasBestCost() const
    {
        return infeasStats.bestCost_;
    }

    /**
     * Returns a vector of the average objective value of infeasible
     * individuals, one element per iteration. If there are no infeasible
     * individuals, then ``INT_MAX`` is stored.
     */
    [[nodiscard]] std::vector<size_t> const &infeasAvgCost() const
    {
        return infeasStats.avgCost_;
    }

    /**
     * Returns a vector of the average number of routes among infeasible
     * individuals, one element per iteration. If there are no infeasible
     * individuals, then 0 is stored.
     */
    [[nodiscard]] std::vector<double> const &infeasAvgNumRoutes() const
    {
        return infeasStats.avgNumRoutes_;
    }

    /**
     * Returns a vector of capacity penalties, one element per iteration.
     */
    [[nodiscard]] std::vector<size_t> const &penaltiesCapacity() const
    {
        return penaltiesCapacity_;
    }

    /**
     * Returns a vector of time warp penalties, one element per iteration.
     */
    [[nodiscard]] std::vector<size_t> const &penaltiesTimeWarp() const
    {
        return penaltiesTimeWarp_;
    }

    /**
     * Returns a vector of (runtime, objective)-pairs, one for each time
     * a new, feasible best heuristic solution has been found.
     */
    [[nodiscard]] timedDatapoints const &incumbents() const
    {
        return incumbents_;
    }

    /**
     * Exports the collected statistics as CSV. Only statistics that have been
     * collected for each iteration are exported. Uses `,` as default separator.
     */
    void toCsv(std::string const &path, char const sep = ',') const;
};

#endif  // STATISTICS_H
