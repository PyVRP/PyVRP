#include "GeneticAlgorithm.h"

#include "Individual.h"
#include "LocalSearch.h"
#include "Population.h"
#include "ProblemData.h"
#include "Result.h"
#include "Statistics.h"

#include <chrono>
#include <numeric>
#include <stdexcept>
#include <utility>

Result GeneticAlgorithm::run(StoppingCriterion &stop)
{
    using clock = std::chrono::system_clock;

    Statistics stats;
    size_t iter = 0;

    if (data.nbClients <= 1)
        return {population.getBestFound(), stats, iter, 0.};

    auto start = clock::now();
    while (not stop(population.getBestFound().cost()))
    {
        iter++;

        auto const parents = population.select();
        auto offspring = crossover(parents, data, penaltyManager, rng);
        educate(offspring);

        if (iter % data.config.nbPenaltyManagement == 0)
            updatePenalties();

        if (data.config.collectStatistics)
            stats.collectFrom(population);
    }

    std::chrono::duration<double> runTime = clock::now() - start;
    return {population.getBestFound(), stats, iter, runTime.count()};
}

void GeneticAlgorithm::educate(Individual &indiv)
{
    localSearch.search(indiv);

    if (data.config.shouldIntensify  // only intensify feasible, new best
        && indiv.isFeasible()        // solutions. Cf. also repair below.
        && indiv.cost() < population.getBestFound().cost())
        localSearch.intensify(indiv);

    population.add(indiv);

    loadFeas.push_back(!indiv.hasExcessCapacity());
    timeFeas.push_back(!indiv.hasTimeWarp());

    if (!indiv.isFeasible()  // possibly repair if currently infeasible
        && rng.randint(100) < data.config.repairProbability)
    {
        // Re-run, but penalise infeasibility more using a penalty booster.
        auto const booster = penaltyManager.getPenaltyBooster();
        localSearch.search(indiv);

        if (indiv.isFeasible())
        {
            if (data.config.shouldIntensify
                && indiv.cost() < population.getBestFound().cost())
                localSearch.intensify(indiv);

            population.add(indiv);

            loadFeas.push_back(!indiv.hasExcessCapacity());
            timeFeas.push_back(!indiv.hasTimeWarp());
        }
    }
}

void GeneticAlgorithm::updatePenalties()
{
    double feasLoadPct = std::reduce(loadFeas.begin(), loadFeas.end());
    feasLoadPct /= static_cast<double>(loadFeas.size());

    penaltyManager.updateCapacityPenalty(feasLoadPct);
    loadFeas.clear();

    double feasTimePct = std::reduce(timeFeas.begin(), timeFeas.end());
    feasTimePct /= static_cast<double>(timeFeas.size());

    penaltyManager.updateTimeWarpPenalty(feasTimePct);
    timeFeas.clear();
}

GeneticAlgorithm::GeneticAlgorithm(ProblemData &data,
                                   PenaltyManager &penaltyManager,
                                   XorShift128 &rng,
                                   Population &population,
                                   LocalSearch &localSearch,
                                   CrossoverOperator op)
    : data(data),
      penaltyManager(penaltyManager),
      rng(rng),
      population(population),
      localSearch(localSearch),
      crossover(std::move(op))
{
    loadFeas.reserve(data.config.nbPenaltyManagement);
    timeFeas.reserve(data.config.nbPenaltyManagement);
}
