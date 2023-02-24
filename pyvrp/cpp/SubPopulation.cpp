#include "SubPopulation.h"

SubPopulation::SubPopulation(ProblemData const &data,
                             DiversityMeasure divOp,
                             PopulationParams const &params)
    : data(data), divOp(divOp), params(params)
{
}

void SubPopulation::add(Individual const *individual)
{
    Item::Proximity proximity;

    for (auto &other : items)  // update distance to other individuals
    {
        auto const div = divOp(data, *individual, *other.individual);
        auto cmp = [](auto &elem, auto &value) { return elem.first < value; };

        auto &oProx = other.proximity;
        auto place = std::lower_bound(oProx.begin(), oProx.end(), div, cmp);
        oProx.emplace(place, div, individual);

        auto &iProx = proximity;
        place = std::lower_bound(iProx.begin(), iProx.end(), div, cmp);
        iProx.emplace(place, div, other.individual);
    }

    auto byCost = [](auto &a, auto &b)
    { return a.individual->cost() < b.individual->cost(); };
    items.emplace_back(individual, 0.0, proximity);
    std::sort(items.begin(), items.end(), byCost);

    updateFitness();

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

void SubPopulation::purge()
{
    auto remove = [&](auto &iterator)
    {
        for (auto &[individual, fitness, proximity] : items)
            // Remove individual from other proximities.
            for (size_t idx = 0; idx != proximity.size(); ++idx)
                if (proximity[idx].second == iterator->individual)
                {
                    proximity.erase(proximity.begin() + idx);
                    break;
                }

        // Remove individual from subpopulation.
        items.erase(iterator);
    };

    while (size() > params.minPopSize)
    {
        // Remove duplicates from the subpopulation (if they exist)
        auto const pred = [&](auto &iterator)
        {
            return !iterator.proximity.empty()
                   && *iterator.proximity[1].second == *iterator.individual;
        };

        auto const duplicate = std::find_if(items.begin(), items.end(), pred);

        if (duplicate == items.end())  // there are no more duplicates
            break;

        remove(duplicate);
    }

    while (size() > params.minPopSize)
    {
        // Remove the worst individual (worst in terms of biased fitness)
        updateFitness();

        auto const worstFitness = std::max_element(
            items.begin(),
            items.end(),
            [](auto const &a, auto const &b) { return a.fitness < b.fitness; });

        remove(worstFitness);
    }
}

void SubPopulation::updateFitness()
{
    std::vector<std::pair<double, size_t>> diversity;
    for (size_t rank = 0; rank != size(); rank++)
    {
        auto const dist = avgDistanceClosest(rank);
        diversity.emplace_back(dist, rank);
    }

    std::sort(diversity.begin(), diversity.end(), std::greater<>());

    auto const popSize = static_cast<double>(size());
    auto const nbElite = std::min(params.nbElite, size());

    for (size_t divRank = 0; divRank != size(); divRank++)
    {
        auto const costRank = diversity[divRank].second;
        auto const divWeight = 1 - nbElite / popSize;

        items[costRank].fitness = (costRank + divWeight * divRank) / popSize;
    }
}

double SubPopulation::avgDistanceClosest(size_t idx) const
{
    auto const &item = items[idx];
    auto const maxSize = std::min(item.proximity.size(), params.nbClose);

    if (maxSize == 0)
        return 0.0;  // TODO 0.0 or 1.0?

    auto result = 0.0;

    for (size_t idx = 0; idx != maxSize; ++idx)
        result += item.proximity[idx].first;

    return result / maxSize;
}
