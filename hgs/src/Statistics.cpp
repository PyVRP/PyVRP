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
        subStats.avgDiversity_.push_back(0.);   // 0 as subst. for no diversity
        subStats.bestCost_.push_back(INT_MAX);  // INT_MAX as subst. for inf
        subStats.avgCost_.push_back(INT_MAX);
        subStats.avgNumRoutes_.push_back(0.);
        return;
    }

    auto const popSize = subPop.size();
    subStats.popSize_.push_back(popSize);

    auto const opDiv = [](double val, auto const &sub) {
        return val + sub.indiv->avgBrokenPairsDistanceClosest();
    };
    auto const totalDiv = accumulate(subPop.begin(), subPop.end(), 0., opDiv);
    subStats.avgDiversity_.push_back(totalDiv / popSize);

    subStats.bestCost_.push_back(subPop[0].indiv->cost());

    auto const opCost
        = [](size_t sum, auto const &sub) { return sum + sub.indiv->cost(); };
    auto const totalCost = accumulate(subPop.begin(), subPop.end(), 0, opCost);
    subStats.avgCost_.push_back(totalCost / popSize);

    double numRoutes = 0.0;
    for (auto &wrapper : subPop)
        for (auto &route : wrapper.indiv->getRoutes())
            if (!route.empty())
                numRoutes += 1;

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

    // Penalty statistics
    penaltiesCapacity_.push_back(pop.params.penaltyCapacity);
    penaltiesTimeWarp_.push_back(pop.params.penaltyTimeWarp);

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
        << "feasible avg. diversity" << sep
        << "feasible best objective" << sep
        << "feasible avg. objective" << sep
        << "feasible avg. # routes" << sep
        << "# infeasible" << sep
        << "infeasible avg. diversity" << sep
        << "infeasible best. objective" << sep
        << "infeasible avg. objective" << sep
        << "infeasible avg. # routes" << sep
        << "penalty capacity" << sep
        << "penalty time warp" << '\n';

    for (size_t it = 0; it != numIters_; it++)
    {
        out << runTimes_[it] << sep
            << iterTimes_[it] << sep
            << feasStats.popSize_[it] << sep
            << feasStats.avgDiversity_[it] << sep
            << feasStats.bestCost_[it] << sep
            << feasStats.avgCost_[it] << sep
            << feasStats.avgNumRoutes_[it] << sep
            << infeasStats.popSize_[it] << sep
            << infeasStats.avgDiversity_[it] << sep
            << infeasStats.bestCost_[it] << sep
            << infeasStats.avgCost_[it] << sep
            << infeasStats.avgNumRoutes_[it] << sep
            << penaltiesCapacity_[it] << sep
            << penaltiesTimeWarp_[it] << '\n';
    }
    // clang-format on
}
