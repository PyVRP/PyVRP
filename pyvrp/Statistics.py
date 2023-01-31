from .Population import Population


class Statistics:
    """
    The Statistics object tracks various (population-level) statistics of
    genetic algorithm runs. This can be helpful in analysing the algorithm's
    performance.
    """

    def collect_from(self, population: Population):
        """
        Collects statistics from the given population object.

        Parameters
        ----------
        population
            Population instance to collect statistics from.
        """
        pass


# #include "Statistics.h"
# #include "Population.h"
#
# #include <fstream>
# #include <numeric>
#
# using std::accumulate;
#
# void Statistics::collectSubPopStats(Population const &population,
#                                     Population::SubPopulation const &subPop,
#                                     Statistics::SubPopStats &subStats)
# {
#     if (subPop.empty())
#     {
#         subStats.popSize_.push_back(0);
#         subStats.avgDiversity_.push_back(0.);   // no diversity
#         subStats.bestCost_.push_back(INT_MAX);  // INT_MAX as subst. for inf
#         subStats.avgCost_.push_back(INT_MAX);
#         subStats.avgNumRoutes_.push_back(0.);
#         return;
#     }
#
#     double totalDiv = 0.;
#     size_t totalCost = 0;
#     size_t numRoutes = 0;
#
#     for (auto &wrap : subPop)
#     {
#         auto const &indiv = *wrap.indiv;
#         totalDiv += population.avgDistanceClosest(indiv);
#         totalCost += indiv.cost();
#         numRoutes += indiv.numRoutes();
#     }
#
#     auto const popSize = subPop.size();
#     auto const dPopSize = static_cast<double>(popSize);
#
#     subStats.popSize_.push_back(popSize);
#     subStats.avgDiversity_.push_back(totalDiv / dPopSize);
#     subStats.bestCost_.push_back(subPop[0].indiv->cost());
#     subStats.avgCost_.push_back(totalCost / dPopSize);
#     subStats.avgNumRoutes_.push_back(numRoutes / dPopSize);
# }
#
# void Statistics::collectFrom(Population const &pop)
# {
#     numIters_++;
#
#     auto const now = clock::now();
#
#     std::chrono::duration<double> const runTime = now - start;
#     runTimes_.push_back(runTime.count());
#
#     std::chrono::duration<double> const iterTime = now - lastIter;
#     iterTimes_.push_back(iterTime.count());
#
#     lastIter = clock::now();  // update for next call
#
#     // Population statistics
#     collectSubPopStats(pop, pop.feasible, feasStats);
#     collectSubPopStats(pop, pop.infeasible, infeasStats);
#
#     // Incumbents
#     auto const &best = pop.bestSol;
#
#     if (!best.isFeasible())
#         return;
#
#     if (incumbents_.empty() || best.cost() < incumbents_.back().second)
#     {
#         std::chrono::duration<double> time = clock::now() - start;
#         incumbents_.emplace_back(time.count(), best.cost());
#     }
# }
#
# size_t Statistics::numIters() const { return numIters_; }
#
# std::vector<double> const &Statistics::runTimes() const { return runTimes_; }
#
# std::vector<double> const &Statistics::iterTimes() const { return iterTimes_;
# }
#
# std::vector<size_t> const &Statistics::feasPopSize() const
# {
#     return feasStats.popSize_;
# }
#
# std::vector<double> const &Statistics::feasAvgDiversity() const
# {
#     return feasStats.avgDiversity_;
# }
#
# std::vector<size_t> const &Statistics::feasBestCost() const
# {
#     return feasStats.bestCost_;
# }
#
# std::vector<size_t> const &Statistics::feasAvgCost() const
# {
#     return feasStats.avgCost_;
# }
#
# std::vector<double> const &Statistics::feasAvgNumRoutes() const
# {
#     return feasStats.avgNumRoutes_;
# }
#
# std::vector<size_t> const &Statistics::infeasPopSize() const
# {
#     return infeasStats.popSize_;
# }
#
# std::vector<double> const &Statistics::infeasAvgDiversity() const
# {
#     return infeasStats.avgDiversity_;
# }
#
# std::vector<size_t> const &Statistics::infeasBestCost() const
# {
#     return infeasStats.bestCost_;
# }
#
# std::vector<size_t> const &Statistics::infeasAvgCost() const
# {
#     return infeasStats.avgCost_;
# }
#
# std::vector<double> const &Statistics::infeasAvgNumRoutes() const
# {
#     return infeasStats.avgNumRoutes_;
# }
#
# Statistics::timedDatapoints const &Statistics::incumbents() const
# {
#     return incumbents_;
# }
#
# void Statistics::toCsv(std::string const &path, char const sep) const
# {
#     std::ofstream out(path);
#
#     if (!out)
#         throw std::runtime_error("Could not open " + path);
#
#     // clang-format off
#     out << "total run-time (s)" << sep
#         << "iteration run-time (s)" << sep
#         << "# feasible" << sep
#         << "feasible best objective" << sep
#         << "feasible avg. objective" << sep
#         << "feasible avg. # routes" << sep
#         << "# infeasible" << sep
#         << "infeasible best. objective" << sep
#         << "infeasible avg. objective" << sep
#         << "infeasible avg. # routes" << '\n';
#
#     for (size_t it = 0; it != numIters_; it++)
#     {
#         out << runTimes_[it] << sep
#             << iterTimes_[it] << sep
#             << feasStats.popSize_[it] << sep
#             << feasStats.bestCost_[it] << sep
#             << feasStats.avgCost_[it] << sep
#             << feasStats.avgNumRoutes_[it] << sep
#             << infeasStats.popSize_[it] << sep
#             << infeasStats.bestCost_[it] << sep
#             << infeasStats.avgCost_[it] << sep
#             << infeasStats.avgNumRoutes_[it] << '\n';
#     }
#     // clang-format on
# }


# #ifndef STATISTICS_H
# #define STATISTICS_H
#
# #include "Population.h"
#
# #include <chrono>
# #include <vector>
#
# class Statistics
# {
#     using clock = std::chrono::system_clock;
#     using timedDatapoints = std::vector<std::pair<double, size_t>>;
#
#     clock::time_point start = clock::now();
#     clock::time_point lastIter = clock::now();
#     size_t numIters_ = 0;
#
#     std::vector<double> runTimes_;
#     std::vector<double> iterTimes_;
#
#     struct SubPopStats
#     {
#         std::vector<size_t> popSize_;
#         std::vector<double> avgDiversity_;
#         std::vector<size_t> bestCost_;
#         std::vector<size_t> avgCost_;
#         std::vector<double> avgNumRoutes_;
#     };
#
#     void collectSubPopStats(Population const &population,
#                             Population::SubPopulation const &subPop,
#                             Statistics::SubPopStats &subStats);
#
#     SubPopStats feasStats;
#     SubPopStats infeasStats;
#
#     timedDatapoints incumbents_;
#
# public:
#     /**
#      * Collects population and objective value statistics. This function is
#      * called repeatedly during the genetic algorithm's search, and stores
#      * relevant data for later evaluation.
#      *
#      * @param population  Population object to collect data from.
#      */
#     void collectFrom(Population const &population);
#
#     /**
#      * Returns the total number of iterations.
#      */
#     [[nodiscard]] size_t numIters() const;
#
#     /**
#      * Returns a vector of run times in seconds, one element per iteration.
#      * Each element indicates the time between the current iteration and the
#      * start of the algorithm.
#      */
#     [[nodiscard]] std::vector<double> const &runTimes() const;
#
#     /**
#      * Returns a vector of run times in seconds, one element per iteration.
#      * Each element indicates the time between the current and previous
#      * iteration.
#      */
#     [[nodiscard]] std::vector<double> const &iterTimes() const;
#
#     /**
#      * Returns a vector of the number of feasible individuals in the
#      population,
#      * one element per iteration.
#      */
#     [[nodiscard]] std::vector<size_t> const &feasPopSize() const;
#
#     /**
#      * Returns a vector of the average feasible sub-population diversity, one
#      * element per iteration. The average diversity is computed as the
#      average
#      * broken pairs distance for each individual in the sub-population,
#      compared
#      * to its neighbours (the neighbourhood size is controlled by the
#      * ``nbClose`` setting).
#      */
#     [[nodiscard]] std::vector<double> const &feasAvgDiversity() const;
#
#     /**
#      * Returns a vector of the best objective value of feasible individuals,
#      * one element per iteration. If there are no feasible individuals, then
#      * ``INT_MAX`` is stored.
#      */
#     [[nodiscard]] std::vector<size_t> const &feasBestCost() const;
#
#     /**
#      * Returns a vector of the average objective value of feasible
#      individuals,
#      * one element per iteration. If there are no feasible individuals, then
#      * ``INT_MAX`` is stored.
#      */
#     [[nodiscard]] std::vector<size_t> const &feasAvgCost() const;
#
#     /**
#      * Returns a vector of the average number of routes among feasible
#      * individuals, one element per iteration. If there are no feasible
#      * individuals, then 0 is stored.
#      */
#     [[nodiscard]] std::vector<double> const &feasAvgNumRoutes() const;
#
#     /**
#      * Returns a vector of the number of infeasible individuals in the
#      * population, one element per iteration.
#      */
#     [[nodiscard]] std::vector<size_t> const &infeasPopSize() const;
#
#     /**
#      * Returns a vector of the average infeasible sub-population diversity,
#      one
#      * element per iteration. The average diversity is computed as the
#      average
#      * broken pairs distance for each individual in the sub-population,
#      compared
#      * to its neighbours (the neighbourhood size is controlled by the
#      * ``nbClose`` setting).
#      */
#     [[nodiscard]] std::vector<double> const &infeasAvgDiversity() const;
#
#     /**
#      * Returns a vector of the best objective value of infeasible
#      individuals,
#      * one element per iteration. If there are no infeasible individuals,
#      then
#      * ``INT_MAX`` is stored.
#      */
#     [[nodiscard]] std::vector<size_t> const &infeasBestCost() const;
#
#     /**
#      * Returns a vector of the average objective value of infeasible
#      * individuals, one element per iteration. If there are no infeasible
#      * individuals, then ``INT_MAX`` is stored.
#      */
#     [[nodiscard]] std::vector<size_t> const &infeasAvgCost() const;
#
#     /**
#      * Returns a vector of the average number of routes among infeasible
#      * individuals, one element per iteration. If there are no infeasible
#      * individuals, then 0 is stored.
#      */
#     [[nodiscard]] std::vector<double> const &infeasAvgNumRoutes() const;
#
#     /**
#      * Returns a vector of (runtime, objective)-pairs, one for each time
#      * a new, feasible best heuristic solution has been found.
#      */
#     [[nodiscard]] timedDatapoints const &incumbents() const;
#
#     /**
#      * Exports the collected statistics as CSV. Only statistics that have
#      been
#      * collected for each iteration are exported. Uses `,` as default
#      separator.
#      */
#     void toCsv(std::string const &path, char sep = ',') const;
# };
#
# #endif  // STATISTICS_H
