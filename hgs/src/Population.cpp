#include "Population.h"

#include "Individual.h"
#include "ProblemData.h"

#include <memory>
#include <vector>

void Population::add(Individual const &indiv)
{
    auto &subPop = indiv.isFeasible() ? feasible : infeasible;
    auto indivPtr = std::make_unique<Individual>(indiv);

    for (auto const &other : subPop)  // update distance to other individuals
        indivPtr->registerNearbyIndividual(other.indiv.get());

    subPop.push_back({std::move(indivPtr), 0});
    updateBiasedFitness(subPop);

    // Trigger a survivor selection if the maximum population size is exceeded
    if (subPop.size() > data.config.minPopSize + data.config.generationSize)
    {
        while (subPop.size() > data.config.minPopSize)  // remove duplicates,
            if (!removeDuplicate(subPop))               // if any exist
                break;

        while (subPop.size() > data.config.minPopSize)
        {
            updateBiasedFitness(subPop);
            removeWorstBiasedFitness(subPop);
        }
    }

    if (indiv.isFeasible() && indiv.cost() < bestSol.cost())
        bestSol = indiv;
}

void Population::updateBiasedFitness(SubPopulation &subPop) const
{
    // Sort population by ascending cost
    std::sort(subPop.begin(), subPop.end(), [](auto &a, auto &b) {
        return a.indiv->cost() < b.indiv->cost();
    });

    // Ranking the individuals based on their diversity contribution (decreasing
    // order of broken pairs distance)
    std::vector<std::pair<double, size_t>> diversity;
    for (size_t rank = 0; rank != subPop.size(); rank++)
    {
        auto const dist = subPop[rank].indiv->avgBrokenPairsDistanceClosest();
        diversity.emplace_back(dist, rank);
    }

    std::sort(diversity.begin(), diversity.end(), std::greater<>());

    auto const popSize = static_cast<double>(subPop.size());
    auto const nbElite = std::min(data.config.nbElite, subPop.size());

    for (size_t divRank = 0; divRank != subPop.size(); divRank++)
    {
        // Ranking the individuals based on the cost and diversity rank
        auto const costRank = diversity[divRank].second;
        auto const divWeight = 1 - nbElite / popSize;

        subPop[costRank].fitness = (costRank + divWeight * divRank) / popSize;
    }
}

bool Population::removeDuplicate(SubPopulation &subPop)
{
    for (auto it = subPop.begin(); it != subPop.end(); ++it)
        if (it->indiv->hasClone())
        {
            subPop.erase(it);
            return true;
        }

    return false;
}

void Population::removeWorstBiasedFitness(SubPopulation &subPop)
{
    auto const &worstFitness = std::max_element(
        subPop.begin(), subPop.end(), [](auto const &a, auto const &b) {
            return a.fitness < b.fitness;
        });

    subPop.erase(worstFitness);
}

Individual const *Population::getBinaryTournament()
{
    auto const fSize = feasible.size();
    auto const popSize = fSize + infeasible.size();

    auto const idx1 = rng.randint(popSize);
    auto &wrap1 = idx1 < fSize ? feasible[idx1] : infeasible[idx1 - fSize];

    auto const idx2 = rng.randint(popSize);
    auto &wrap2 = idx2 < fSize ? feasible[idx2] : infeasible[idx2 - fSize];

    return (wrap1.fitness < wrap2.fitness ? wrap1.indiv : wrap2.indiv).get();
}

std::pair<Individual const *, Individual const *> Population::selectParents()
{
    auto const *par1 = getBinaryTournament();
    auto const *par2 = getBinaryTournament();

    auto const lowerBound = data.config.lbDiversity * data.nbClients;
    auto const upperBound = data.config.ubDiversity * data.nbClients;
    auto diversity = par1->brokenPairsDistance(par2);

    size_t tries = 1;
    while ((diversity < lowerBound || diversity > upperBound) && tries++ < 10)
    {
        par2 = getBinaryTournament();
        diversity = par1->brokenPairsDistance(par2);
    }

    return std::make_pair(par1, par2);
}

Individual const &Population::getBestFound() const { return bestSol; }

Population::Population(ProblemData &data, XorShift128 &rng)
    : data(data), rng(rng), bestSol(data, rng)  // random initial best solution
{
    // Generate minPopSize random individuals to seed the population.
    for (size_t count = 0; count != data.config.minPopSize; ++count)
    {
        Individual randomIndiv(data, rng);
        add(randomIndiv);
    }
}
