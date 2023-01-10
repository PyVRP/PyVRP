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
        registerNearbyIndividual(indivPtr.get(), other.indiv.get());

    subPop.push_back({std::move(indivPtr), 0});
    updateBiasedFitness(subPop);

    if (subPop.size() > minPopSize + generationSize)
        purge(subPop);  // survivor selection

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
        auto dist = avgDistanceClosest(*subPop[rank].indiv.get());
        diversity.emplace_back(dist, rank);
    }

    std::sort(diversity.begin(), diversity.end(), std::greater<>());

    for (size_t divRank = 0; divRank != subPop.size(); divRank++)
    {
        auto const popSize = static_cast<double>(subPop.size());

        // Ranking the individuals based on the cost and diversity rank
        auto const costRank = diversity[divRank].second;
        auto const divWeight = 1 - std::min(nbElite, subPop.size()) / popSize;

        subPop[costRank].fitness = (costRank + divWeight * divRank) / popSize;
    }
}

void Population::purge(std::vector<IndividualWrapper> &subPop)
{
    auto remove = [&](auto &iterator) {
        auto const *indiv = iterator->indiv.get();

        for (auto [_, individuals] : proximity)
            for (size_t idx = 0; idx != individuals.size(); ++idx)
                if (individuals[idx].second == indiv)
                {
                    individuals.erase(individuals.begin() + idx);
                    break;
                }

        proximity.erase(indiv);
        subPop.erase(iterator);
    };

    while (subPop.size() > minPopSize)
    {
        // Remove duplicates from the subpopulation (if they exist)
        auto const pred = [&](auto &it) {
            return proximity.contains(it.indiv.get())
                   && !proximity.at(it.indiv.get()).empty()
                   && proximity.at(it.indiv.get()).begin()->first == 0;
        };

        auto const duplicate = std::find_if(subPop.begin(), subPop.end(), pred);

        if (duplicate == subPop.end())  // there are no more duplicates
            break;

        remove(duplicate);
    }

    while (subPop.size() > minPopSize)
    {
        // Remove the worst individual (worst in terms of biased fitness)
        updateBiasedFitness(subPop);

        auto const worstFitness = std::max_element(
            subPop.begin(), subPop.end(), [](auto const &a, auto const &b) {
                return a.fitness < b.fitness;
            });

        remove(worstFitness);
    }
}

void Population::registerNearbyIndividual(Individual *first, Individual *second)
{
    auto const dist = divOp(data, *first, *second);
    auto cmp = [](auto &elem, auto &value) { return elem.first < value; };

    auto &fProx = proximity[first];
    auto place = std::lower_bound(fProx.begin(), fProx.end(), dist, cmp);
    fProx.emplace(place, dist, second);

    auto &sProx = proximity[second];
    place = std::lower_bound(sProx.begin(), sProx.end(), dist, cmp);
    sProx.emplace(place, dist, first);
}

double Population::avgDistanceClosest(Individual const &indiv) const
{
    if (!proximity.contains(&indiv) || proximity.at(&indiv).empty())
        return 0.;

    auto const &prox = proximity.at(&indiv);
    auto const maxSize = std::min(nbClose, prox.size());
    auto start = prox.begin();
    int result = 0;

    for (auto it = start; it != start + maxSize; ++it)
        result += it->first;

    return result / static_cast<double>(maxSize);
}

Individual const *Population::getBinaryTournament()
{
    auto const fSize = numFeasible();

    auto const idx1 = rng.randint(size());
    auto &wrap1 = idx1 < fSize ? feasible[idx1] : infeasible[idx1 - fSize];

    auto const idx2 = rng.randint(size());
    auto &wrap2 = idx2 < fSize ? feasible[idx2] : infeasible[idx2 - fSize];

    return (wrap1.fitness < wrap2.fitness ? wrap1.indiv : wrap2.indiv).get();
}

std::pair<Individual const *, Individual const *> Population::select()
{
    auto const *par1 = getBinaryTournament();
    auto const *par2 = getBinaryTournament();

    auto diversity = divOp(data, *par1, *par2);

    size_t tries = 1;
    while ((diversity < lbDiversity || diversity > ubDiversity) && tries++ < 10)
    {
        par2 = getBinaryTournament();
        diversity = divOp(data, *par1, *par2);
    }

    return std::make_pair(par1, par2);
}

size_t Population::size() const { return numFeasible() + numInfeasible(); }

size_t Population::numFeasible() const { return feasible.size(); }

size_t Population::numInfeasible() const { return infeasible.size(); }

Individual const &Population::getBestFound() const { return bestSol; }

Population::Population(ProblemData const &data,
                       PenaltyManager const &penaltyManager,
                       XorShift128 &rng,
                       DiversityMeasure op,
                       size_t minPopSize,
                       size_t generationSize,
                       size_t nbElite,
                       size_t nbClose,
                       double lbDiversity,
                       double ubDiversity)
    : data(data),
      rng(rng),
      divOp(std::move(op)),
      minPopSize(minPopSize),
      generationSize(generationSize),
      nbElite(nbElite),
      nbClose(nbClose),
      lbDiversity(lbDiversity),
      ubDiversity(ubDiversity),
      bestSol(data, penaltyManager, rng)  // random initial best solution
{
    // Generate minPopSize random individuals to seed the population.
    for (size_t count = 0; count != minPopSize; ++count)
        add({data, penaltyManager, rng});
}
