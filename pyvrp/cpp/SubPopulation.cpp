#include "SubPopulation.h"

#include <numeric>

SubPopulation::SubPopulation(ProblemData const &data,
                             DiversityMeasure divOp,
                             PopulationParams const &params)
    : data(data), divOp(divOp), params(params)
{
}

SubPopulation::~SubPopulation()
{
    for (auto &item : items)
        delete item.individual;
}

void SubPopulation::add(Individual const *individual)
{
    // Copy the given individual into a new memory location, and use that from
    // now on.
    individual = new Individual(*individual);
    Item item = {&params, individual, 0.0, {}};

    for (auto &other : items)  // update distance to other individuals
    {
        auto const div = divOp(data, *individual, *other.individual);
        auto cmp = [](auto &elem, auto &value) { return elem.first < value; };

        auto &oProx = other.proximity;
        auto place = std::lower_bound(oProx.begin(), oProx.end(), div, cmp);
        oProx.emplace(place, div, individual);

        auto &iProx = item.proximity;
        place = std::lower_bound(iProx.begin(), iProx.end(), div, cmp);
        iProx.emplace(place, div, other.individual);
    }

    items.push_back(item);  // add individual and update fitness scores for the
    updateFitness();        // entire subpopulation.

    if (size() > params.maxPopSize())
        purge();
}

size_t SubPopulation::size() const { return items.size(); }

SubPopulation::Item const &SubPopulation::operator[](size_t idx) const
{
    return items[idx];
}

std::vector<SubPopulation::Item>::const_iterator SubPopulation::cbegin() const
{
    return items.cbegin();
}

std::vector<SubPopulation::Item>::const_iterator SubPopulation::cend() const
{
    return items.cend();
}

void SubPopulation::remove(
    std::vector<SubPopulation::Item>::iterator const &iterator)
{
    for (auto &[params, individual, fitness, proximity] : items)
        // Remove individual from other proximities.
        for (size_t idx = 0; idx != proximity.size(); ++idx)
            if (proximity[idx].second == iterator->individual)
            {
                proximity.erase(proximity.begin() + idx);
                break;
            }

    delete iterator->individual;  // dispose of manually allocated memory
    items.erase(iterator);        // before the item is removed. Then update
    updateFitness();              // the fitness scores.
}

void SubPopulation::purge()
{
    while (size() > params.minPopSize)
    {
        // Remove duplicates from the subpopulation (if they exist)
        auto const pred = [&](auto &iterator)
        {
            return !iterator.proximity.empty()
                   && *iterator.proximity[0].second == *iterator.individual;
        };

        auto const duplicate = std::find_if(items.begin(), items.end(), pred);

        if (duplicate == items.end())  // there are no more duplicates
            break;

        remove(duplicate);
    }

    while (size() > params.minPopSize)
    {
        auto const worstFitness = std::max_element(
            items.begin(),
            items.end(),
            [](auto const &a, auto const &b) { return a.fitness < b.fitness; });

        remove(worstFitness);
    }
}

// TODO this function should also be called after updating the penalties, since
//  in that case the cost rank likely changes.
void SubPopulation::updateFitness()
{
    if (items.empty())
        return;

    std::vector<size_t> byCost(size());
    std::iota(byCost.begin(), byCost.end(), 0);

    std::stable_sort(
        byCost.begin(),
        byCost.end(),
        [&](size_t a, size_t b)
        { return items[a].individual->cost() < items[b].individual->cost(); });

    std::vector<std::pair<double, size_t>> diversity;
    for (size_t costRank = 0; costRank != size(); costRank++)
    {
        auto const dist = items[byCost[costRank]].avgDistanceClosest();
        diversity.emplace_back(dist, costRank);
    }

    std::stable_sort(diversity.begin(), diversity.end(), std::greater<>());

    auto const popSize = static_cast<double>(size());
    auto const nbElite = std::min(params.nbElite, size());
    auto const divWeight = 1 - nbElite / popSize;

    for (size_t divRank = 0; divRank != size(); divRank++)
    {
        auto const costRank = diversity[divRank].second;
        auto const idx = byCost[costRank];
        items[idx].fitness = (costRank + divWeight * divRank) / popSize;
    }
}

double SubPopulation::Item::avgDistanceClosest() const
{
    auto const maxSize = std::min(proximity.size(), params->nbClose);
    auto result = 0.0;

    for (size_t idx = 0; idx != maxSize; ++idx)
        result += proximity[idx].first;

    return result / std::max(maxSize, size_t(1));
}
