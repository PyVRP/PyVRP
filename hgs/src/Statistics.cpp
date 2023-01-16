#include "Statistics.h"
#include "Population.h"

#include <fstream>
#include <numeric>

using std::accumulate;

void Statistics::collectSubPopStats(Population const &population,
                                    Population::SubPopulation const &subPop,
                                    Statistics::SubPopStats &subStats)
{
    if (subPop.empty())
    {
        subStats.popSize_.push_back(0);
        subStats.avgDiversity_.push_back(0.);   // no diversity
        subStats.bestCost_.push_back(INT_MAX);  // INT_MAX as subst. for inf
        subStats.avgCost_.push_back(INT_MAX);
        subStats.avgNumRoutes_.push_back(0.);
        return;
    }

    double totalDiv = 0.;
    size_t totalCost = 0;
    size_t numRoutes = 0;

    for (auto &wrap : subPop)
    {
        auto const &indiv = *wrap.indiv;
        totalDiv += population.avgDistanceClosest(indiv);
        totalCost += indiv.cost();
        numRoutes += indiv.numRoutes();
    }

    auto const popSize = subPop.size();
    auto const dPopSize = static_cast<double>(popSize);

    subStats.popSize_.push_back(popSize);
    subStats.avgDiversity_.push_back(totalDiv / dPopSize);
    subStats.bestCost_.push_back(subPop[0].indiv->cost());
    subStats.avgCost_.push_back(totalCost / dPopSize);
    subStats.avgNumRoutes_.push_back(numRoutes / dPopSize);
}

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
    collectSubPopStats(pop, pop.feasible, feasStats);
    collectSubPopStats(pop, pop.infeasible, infeasStats);

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

size_t Statistics::numIters() const { return numIters_; }

std::vector<double> const &Statistics::runTimes() const { return runTimes_; }

std::vector<double> const &Statistics::iterTimes() const { return iterTimes_; }

std::vector<size_t> const &Statistics::feasPopSize() const
{
    return feasStats.popSize_;
}

std::vector<double> const &Statistics::feasAvgDiversity() const
{
    return feasStats.avgDiversity_;
}

std::vector<size_t> const &Statistics::feasBestCost() const
{
    return feasStats.bestCost_;
}

std::vector<size_t> const &Statistics::feasAvgCost() const
{
    return feasStats.avgCost_;
}

std::vector<double> const &Statistics::feasAvgNumRoutes() const
{
    return feasStats.avgNumRoutes_;
}

std::vector<size_t> const &Statistics::infeasPopSize() const
{
    return infeasStats.popSize_;
}

std::vector<double> const &Statistics::infeasAvgDiversity() const
{
    return infeasStats.avgDiversity_;
}

std::vector<size_t> const &Statistics::infeasBestCost() const
{
    return infeasStats.bestCost_;
}

std::vector<size_t> const &Statistics::infeasAvgCost() const
{
    return infeasStats.avgCost_;
}

std::vector<double> const &Statistics::infeasAvgNumRoutes() const
{
    return infeasStats.avgNumRoutes_;
}

Statistics::timedDatapoints const &Statistics::incumbents() const
{
    return incumbents_;
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
