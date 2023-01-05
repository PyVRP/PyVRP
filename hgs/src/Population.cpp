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

    if (subPop.size() > data.config.minPopSize + data.config.generationSize)
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

void Population::purge(std::vector<IndividualWrapper> &subPop)
{
    while (subPop.size() > data.config.minPopSize)
    {
        // Remove duplicates from the sub-population (if they exist)
        auto const pred = [](auto &it) {
            return !it.indiv->indivsByProximity.empty()
                   && it.indiv->indivsByProximity.begin()->first == 0;
        };

        auto const duplicate = std::find_if(subPop.begin(), subPop.end(), pred);

        if (duplicate == subPop.end())  // there are no more duplicates
            break;

        subPop.erase(duplicate);
    }

    while (subPop.size() > data.config.minPopSize)
    {
        // Remove the worst individual (worst in terms of biased fitness)
        updateBiasedFitness(subPop);

        auto const &worstFitness = std::max_element(
            subPop.begin(), subPop.end(), [](auto const &a, auto const &b) {
                return a.fitness < b.fitness;
            });

        subPop.erase(worstFitness);
    }
}

void Population::registerNearbyIndividual(Individual *first,
                                          Individual *second) const
{
    auto const dist = divOp(data, *first, *second);
    auto cmp = [](auto &elem, auto &value) { return elem.first < value; };

    auto &fProx = first->indivsByProximity;
    auto place = std::lower_bound(fProx.begin(), fProx.end(), dist, cmp);
    fProx.emplace(place, dist, second);

    auto &sProx = second->indivsByProximity;
    place = std::lower_bound(sProx.begin(), sProx.end(), dist, cmp);
    sProx.emplace(place, dist, first);
}

double Population::avgDistanceClosest(Individual const &indiv) const
{
    if (indiv.indivsByProximity.empty())
        return 0.;

    auto maxSize
        = std::min(data.config.nbClose, indiv.indivsByProximity.size());
    auto start = indiv.indivsByProximity.begin();
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

    auto const lowerBound = data.config.lbDiversity;
    auto const upperBound = data.config.ubDiversity;
    auto diversity = divOp(data, *par1, *par2);

    size_t tries = 1;
    while ((diversity < lowerBound || diversity > upperBound) && tries++ < 10)
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
                       XorShift128 &rng,
                       DiversityMeasure op)
    : data(data),
      rng(rng),
      divOp(std::move(op)),
      bestSol(data, rng)  // random initial best solution
{
    // Generate minPopSize random individuals to seed the population.
    for (size_t count = 0; count != data.config.minPopSize; ++count)
        add({data, rng});
}
