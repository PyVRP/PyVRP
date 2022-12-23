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

Result GeneticAlgorithm::run(StoppingCriterion &stop)
{
    using clock = std::chrono::system_clock;

    if (operators.empty())
        throw std::runtime_error("Cannot run genetic algorithm without "
                                 "crossover operators.");

    Statistics stats;
    size_t iter = 0;

    if (data.nbClients <= 1)
        return {population.getBestFound(), stats, iter, 0.};

    auto start = clock::now();
    while (not stop(population.getBestFound().cost()))
    {
        iter++;

        auto offspring = crossover();
        educate(offspring);

        // Diversification and penalty management
        if (iter % data.config.nbPenaltyManagement == 0)
        {
            updatePenalties();
            population.reorder();  // re-order since penalties have changed
        }

        if (data.config.collectStatistics)
            stats.collectFrom(population);
    }

    std::chrono::duration<double> runTime = clock::now() - start;
    return {population.getBestFound(), stats, iter, runTime.count()};
}

Individual GeneticAlgorithm::crossover() const
{
    auto const parents = population.selectParents();

    std::vector<Individual> offspring;
    offspring.reserve(operators.size());

    for (auto const &op : operators)
        offspring.push_back(op(parents, data, rng));

    // A simple geometric acceptance criterion: select the best with some
    // probability. If not accepted, test the second best, etc.
    std::sort(offspring.begin(),
              offspring.end(),
              [](auto const &indiv1, auto const &indiv2) {
                  return indiv1.cost() < indiv2.cost();
              });

    for (auto &indiv : offspring)
        if (rng.randint(100) < data.config.selectProbability)
            return indiv;

    return offspring.back();  // fallback in case no offspring were selected
}

void GeneticAlgorithm::educate(Individual &indiv)
{
    localSearch.search(indiv);

    if (data.config.shouldIntensify  // only intensify feasible, new best
        && indiv.isFeasible()        // solutions. Cf. also repair below.
        && indiv.cost() < population.getBestFound().cost())
        localSearch.intensify(indiv);

    population.addIndividual(indiv);

    loadFeas.push_back(!indiv.hasExcessCapacity());
    timeFeas.push_back(!indiv.hasTimeWarp());

    if (!indiv.isFeasible()  // possibly repair if currently infeasible
        && rng.randint(100) < data.config.repairProbability)
    {
        // Re-run, but penalise infeasibility more using a penalty booster.
        auto const booster = data.pManager.getPenaltyBooster();
        localSearch.search(indiv);

        if (indiv.isFeasible())
        {
            if (data.config.shouldIntensify
                && indiv.cost() < population.getBestFound().cost())
                localSearch.intensify(indiv);

            population.addIndividual(indiv);

            loadFeas.push_back(!indiv.hasExcessCapacity());
            timeFeas.push_back(!indiv.hasTimeWarp());
        }
    }
}

void GeneticAlgorithm::updatePenalties()
{
    double feasLoadPct = std::reduce(loadFeas.begin(), loadFeas.end());
    feasLoadPct /= static_cast<double>(loadFeas.size());

    data.pManager.updateCapacityPenalty(feasLoadPct);
    loadFeas.clear();

    double feasTimePct = std::reduce(timeFeas.begin(), timeFeas.end());
    feasTimePct /= static_cast<double>(timeFeas.size());

    data.pManager.updateTimeWarpPenalty(feasTimePct);
    timeFeas.clear();
}

GeneticAlgorithm::GeneticAlgorithm(ProblemData &data,
                                   XorShift128 &rng,
                                   Population &population,
                                   LocalSearch &localSearch)
    : data(data), rng(rng), population(population), localSearch(localSearch)
{
    loadFeas.reserve(data.config.nbPenaltyManagement);
    timeFeas.reserve(data.config.nbPenaltyManagement);
}
