#include "SubPopulation.h"

#include <numeric>

using pyvrp::SubPopulation;
using const_iter = std::vector<SubPopulation::Item>::const_iterator;
using iter = std::vector<SubPopulation::Item>::iterator;

SubPopulation::SubPopulation(diversity::DiversityMeasure divOp,
                             PopulationParams const &params)
    : divOp(divOp), params(params)
{
}

SubPopulation::~SubPopulation()
{
    for (auto &item : items)
        delete item.solution;
}

void SubPopulation::add(Solution const *solution,
                        CostEvaluator const &costEvaluator)
{
    // Copy the given solution into a new memory location, and use that from
    // now on.
    solution = new Solution(*solution);
    Item item = {&params, solution, 0.0, {}};

    for (auto &other : items)  // update distance to other solutions
    {
        auto const div = divOp(*solution, *other.solution);
        auto cmp = [](auto &elem, auto &value) { return elem.first < value; };

        auto &oProx = other.proximity;
        auto place = std::lower_bound(oProx.begin(), oProx.end(), div, cmp);
        oProx.emplace(place, div, solution);

        auto &iProx = item.proximity;
        place = std::lower_bound(iProx.begin(), iProx.end(), div, cmp);
        iProx.emplace(place, div, other.solution);
    }

    items.push_back(item);  // add solution

    if (size() > params.maxPopSize())
        purge(costEvaluator);
}

size_t SubPopulation::size() const { return items.size(); }

SubPopulation::Item const &SubPopulation::operator[](size_t idx) const
{
    return items[idx];
}

const_iter SubPopulation::cbegin() const { return items.cbegin(); }

const_iter SubPopulation::cend() const { return items.cend(); }

void SubPopulation::remove(iter const &iterator)
{
    for (auto &[params, solution, fitness, proximity] : items)
        // Remove solution from other proximities.
        for (size_t idx = 0; idx != proximity.size(); ++idx)
            if (proximity[idx].second == iterator->solution)
            {
                proximity.erase(proximity.begin() + idx);
                break;
            }

    delete iterator->solution;  // dispose of manually allocated memory
    items.erase(iterator);      // before the item is removed.
}

void SubPopulation::purge(CostEvaluator const &costEvaluator)
{
    // First we remove duplicates. This does not rely on the fitness values.
    while (size() > params.minPopSize)
    {
        // Remove duplicates from the subpopulation (if they exist)
        auto const pred = [&](auto &iterator) {
            return !iterator.proximity.empty()
                   && *iterator.proximity[0].second == *iterator.solution;
        };

        auto const duplicate = std::find_if(items.begin(), items.end(), pred);

        if (duplicate == items.end())  // there are no more duplicates
            break;

        remove(duplicate);
    }

    while (size() > params.minPopSize)
    {
        // Before using fitness, we must update fitness
        updateFitness(costEvaluator);
        auto const worstFitness = std::max_element(
            items.begin(), items.end(), [](auto const &a, auto const &b) {
                return a.fitness < b.fitness;
            });

        remove(worstFitness);
    }
}

void SubPopulation::updateFitness(CostEvaluator const &costEvaluator)
{
    if (items.empty())
        return;

    std::vector<size_t> byCost(size());
    std::iota(byCost.begin(), byCost.end(), 0);

    std::stable_sort(byCost.begin(), byCost.end(), [&](size_t a, size_t b) {
        return costEvaluator.penalisedCost(*items[a].solution)
               < costEvaluator.penalisedCost(*items[b].solution);
    });

    std::vector<std::pair<double, size_t>> diversity;
    for (size_t costRank = 0; costRank != size(); costRank++)
    {
        auto const dist = items[byCost[costRank]].avgDistanceClosest();
        diversity.emplace_back(-dist, costRank);  // higher is better
    }

    std::stable_sort(diversity.begin(), diversity.end());

    auto const popSize = static_cast<double>(size());
    auto const nbElite = std::min(params.nbElite, size());
    auto const divWeight = 1 - nbElite / popSize;

    for (size_t divRank = 0; divRank != size(); divRank++)
    {
        auto const costRank = diversity[divRank].second;
        auto const idx = byCost[costRank];
        items[idx].fitness = (costRank + divWeight * divRank) / (2 * popSize);
    }
}

double SubPopulation::Item::avgDistanceClosest() const
{
    auto const maxSize = std::min(proximity.size(), params->nbClose);
    auto result = 0.0;

    for (size_t idx = 0; idx != maxSize; ++idx)
        result += proximity[idx].first;

    return result / std::max<size_t>(maxSize, 1);
}
