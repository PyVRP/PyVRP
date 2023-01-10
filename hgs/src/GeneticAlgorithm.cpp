#include "GeneticAlgorithm.h"

#include "Individual.h"
#include "LocalSearch.h"
#include "Population.h"
#include "ProblemData.h"
#include "Result.h"
#include "Statistics.h"

#include <chrono>
#include <numeric>
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

        if (iter % nbPenaltyManagement == 0)
            updatePenalties();

        if (collectStatistics)
            stats.collectFrom(population);
    }

    std::chrono::duration<double> runTime = clock::now() - start;
    return {population.getBestFound(), stats, iter, runTime.count()};
}

void GeneticAlgorithm::educate(Individual &indiv)
{
    localSearch.search(indiv);

    if (shouldIntensify        // only intensify feasible, new best
        && indiv.isFeasible()  // solutions. Cf. also repair below.
        && indiv.cost() < population.getBestFound().cost())
        localSearch.intensify(indiv);

    population.add(indiv);

    loadFeas.push_back(!indiv.hasExcessCapacity());
    timeFeas.push_back(!indiv.hasTimeWarp());

    // Possibly repair if current solution is infeasible. In that case, we
    // penalise infeasibility more using a penalty booster.
    if (!indiv.isFeasible() && rng.randint(100) < repairProbability)
    {
        auto const booster = penaltyManager.getPenaltyBooster();
        localSearch.search(indiv);

        if (indiv.isFeasible())
        {
            if (shouldIntensify
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
                                   CrossoverOperator op,
                                   size_t nbPenaltyManagement,
                                   bool collectStatistics,
                                   bool shouldIntensify,
                                   size_t repairProbability)
    : data(data),
      penaltyManager(penaltyManager),
      rng(rng),
      population(population),
      localSearch(localSearch),
      crossover(std::move(op)),
      nbPenaltyManagement(nbPenaltyManagement),
      collectStatistics(collectStatistics),
      shouldIntensify(shouldIntensify),
      repairProbability(repairProbability)
{
    loadFeas.reserve(nbPenaltyManagement);
    timeFeas.reserve(nbPenaltyManagement);
}
