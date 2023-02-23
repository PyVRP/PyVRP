#include "SubPopulation.h"

SubPopulation::SubPopulation(ProblemData const &data,
                             DiversityMeasure const &divOp,
                             PopulationParams const &params)
    : data(data), divOp(divOp), params(params)
{
}

void SubPopulation::add(Individual const *individual)
{
    Item item = {individual, 0.0, {}};

    for (auto const &other : items)  // update distance to other individuals
    {
        auto const div = divOp(data, *individual, *other.individual);
        auto cmp = [](auto &elem, auto &value) { return elem.second < value; };

        auto &oProx = other.proximity;
        auto place = std::lower_bound(oProx.begin(), oProx.end(), div, cmp);
        oProx.emplace(place, individual, div);

        auto &tProx = item.proximity;
        place = std::lower_bound(tProx.begin(), tProx.end(), div, cmp);
        tProx.emplace(place, other.individual, div);
    }

    items.push_back(item);
    updateFitness();

    if (size() > params.maxPopSize())
        purge();
}

size_t SubPopulation::size() const { return items.size(); }

Individual const *SubPopulation::operator[](size_t idx) const
{
    return items[idx].individual;
}

void SubPopulation::purge()
{
    auto remove = [&](auto &iterator)
    {
        for (auto [_, _, proximity] : items)
            // Remove individual from other proximities.
            for (size_t idx = 0; idx != proximity.size(); ++idx)
                if (proximity[idx].first == iterator->individual)
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
                   && iterator.proximity[1].first == iterator.individual;
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
    for (size_t idx = 0; idx != size(); idx++)
    {
        auto const dist = avgDistanceClosest(idx);
        diversity.emplace_back(dist, idx);
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
        result += item.proximity[idx].second;

    return result / maxSize;
}
