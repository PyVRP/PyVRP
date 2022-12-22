#include "GeneticAlgorithm.h"

#include "Individual.h"
#include "LocalSearch.h"
#include "Params.h"
#include "Population.h"
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

    if (params.nbClients <= 1)
        return {population.getBestFound(), stats, iter, 0.};

    auto start = clock::now();
    while (not stop(population.getBestFound().cost()))
    {
        iter++;

        auto offspring = crossover();
        educate(offspring);

        // Diversification and penalty management
        if (iter % params.config.nbPenaltyManagement == 0)
        {
            updatePenalties();
            population.reorder();  // re-order since penalties have changed
        }

        if (params.config.collectStatistics)
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
        offspring.push_back(op(parents, params, rng));

    // A simple geometric acceptance criterion: select the best with some
    // probability. If not accepted, test the second best, etc.
    std::sort(offspring.begin(), offspring.end());

    for (auto &indiv : offspring)
        if (rng.randint(100) < params.config.selectProbability)
            return indiv;

    return offspring.back();  // fallback in case no offspring were selected
}

void GeneticAlgorithm::educate(Individual &indiv)
{
    localSearch.search(indiv);

    if (params.config.shouldIntensify  // only intensify feasible, new best
        && indiv.isFeasible()          // solutions. Cf. also repair below.
        && indiv < population.getBestFound())
        localSearch.intensify(indiv);

    population.addIndividual(indiv);

    loadFeas.push_back(!indiv.hasExcessCapacity());
    timeFeas.push_back(!indiv.hasTimeWarp());

    if (!indiv.isFeasible()  // possibly repair if currently infeasible
        && rng.randint(100) < params.config.repairProbability)
    {
        // Re-run, but penalise infeasibility more using a penalty booster.
        auto const booster = params.pManager.getPenaltyBooster();
        localSearch.search(indiv);

        if (indiv.isFeasible())
        {
            if (params.config.shouldIntensify
                && indiv < population.getBestFound())
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

    params.pManager.updateCapacityPenalty(feasLoadPct);
    loadFeas.clear();

    double feasTimePct = std::reduce(timeFeas.begin(), timeFeas.end());
    feasTimePct /= static_cast<double>(timeFeas.size());

    params.pManager.updateTimeWarpPenalty(feasTimePct);
    timeFeas.clear();
}

GeneticAlgorithm::GeneticAlgorithm(Params &params,
                                   XorShift128 &rng,
                                   Population &population,
                                   LocalSearch &localSearch)
    : params(params), rng(rng), population(population), localSearch(localSearch)
{
    loadFeas.reserve(params.config.nbPenaltyManagement);
    timeFeas.reserve(params.config.nbPenaltyManagement);
}
