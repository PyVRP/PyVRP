#include "Statistics.h"
#include "Population.h"

#include <fstream>
#include <numeric>

namespace
{
using std::accumulate;

void collectSubPopStats(Population::SubPopulation const &subPop,
                        Statistics::SubPopStats &subStats)
{
    if (subPop.empty())
    {
        subStats.popSize_.push_back(0);
        subStats.bestCost_.push_back(INT_MAX);  // INT_MAX as subst. for inf
        subStats.avgCost_.push_back(INT_MAX);
        subStats.avgNumRoutes_.push_back(0.);
        return;
    }

    auto const popSize = subPop.size();
    subStats.popSize_.push_back(popSize);
    subStats.bestCost_.push_back(subPop[0].indiv->cost());

    auto const opCost
        = [](size_t sum, auto const &sub) { return sum + sub.indiv->cost(); };
    auto const totalCost = accumulate(subPop.begin(), subPop.end(), 0, opCost);
    subStats.avgCost_.push_back(totalCost / popSize);

    double numRoutes = 0.0;
    for (auto &wrapper : subPop)
        numRoutes += wrapper.indiv->numRoutes();

    subStats.avgNumRoutes_.push_back(numRoutes / popSize);
}
}  // namespace

void Statistics::collectFrom(Population const &pop)
{
    numIters_++;

    auto const now = clock::now();

    std::chrono::duration<double> const runTime = now - start;
    runTimes_.push_back(runTime.count());

    std::chrono::duration<double> const iterTime = now - lastIter;
    iterTimes_.push_back(iterTime.count());

    lastIter = clock::now();  // update for next call

    // Population statistics
    collectSubPopStats(pop.feasible, feasStats);
    collectSubPopStats(pop.infeasible, infeasStats);

    // Incumbents
    auto const &best = pop.bestSol;

    if (!best.isFeasible())
        return;

    if (incumbents_.empty() || best.cost() < incumbents_.back().second)
    {
        std::chrono::duration<double> time = clock::now() - start;
        incumbents_.emplace_back(time.count(), best.cost());
    }
}

void Statistics::toCsv(std::string const &path, char const sep) const
{
    std::ofstream out(path);

    if (!out)
        throw std::runtime_error("Could not open " + path);

    // clang-format off
    out << "total run-time (s)" << sep
        << "iteration run-time (s)" << sep
        << "# feasible" << sep
        << "feasible best objective" << sep
        << "feasible avg. objective" << sep
        << "feasible avg. # routes" << sep
        << "# infeasible" << sep
        << "infeasible best. objective" << sep
        << "infeasible avg. objective" << sep
        << "infeasible avg. # routes" << '\n';

    for (size_t it = 0; it != numIters_; it++)
    {
        out << runTimes_[it] << sep
            << iterTimes_[it] << sep
            << feasStats.popSize_[it] << sep
            << feasStats.bestCost_[it] << sep
            << feasStats.avgCost_[it] << sep
            << feasStats.avgNumRoutes_[it] << sep
            << infeasStats.popSize_[it] << sep
            << infeasStats.bestCost_[it] << sep
            << infeasStats.avgCost_[it] << sep
            << infeasStats.avgNumRoutes_[it] << '\n';
    }
    // clang-format on
}
