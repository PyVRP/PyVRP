#include "SubPopulation.h"

#include <numeric>
#include <stdexcept>

using pyvrp::PopulationParams;
using pyvrp::SubPopulation;
using const_iter = std::vector<SubPopulation::Item>::const_iterator;
using iter = std::vector<SubPopulation::Item>::iterator;

PopulationParams::PopulationParams(size_t minPopSize,
                                   size_t generationSize,
                                   size_t nbElite,
                                   size_t nbClose,
                                   double lbDiversity,
                                   double ubDiversity)
    : minPopSize(minPopSize),
      generationSize(generationSize),
      nbElite(nbElite),
      nbClose(nbClose),
      lbDiversity(lbDiversity),
      ubDiversity(ubDiversity)
{
    if (lbDiversity < 0 || lbDiversity > 1)
        throw std::invalid_argument("lb_diversity must be in [0, 1].");

    if (ubDiversity < 0 || ubDiversity > 1)
        throw std::invalid_argument("ub_diversity must be in [0, 1].");

    if (ubDiversity <= lbDiversity)
    {
        auto const msg = "ub_diversity <= lb_diversity not understood.";
        throw std::invalid_argument(msg);
    }
}

size_t PopulationParams::maxPopSize() const
{
    return minPopSize + generationSize;
}

SubPopulation::SubPopulation(diversity::DiversityMeasure divOp,
                             PopulationParams const &params)
    : divOp(divOp), params(params)
{
}

void SubPopulation::add(std::shared_ptr<Solution const> const &solution,
                        CostEvaluator const &costEvaluator)
{
    Item item = {&params, solution, 0.0, {}};

    for (auto &other : items_)  // update distance to other solutions
    {
        auto const div = divOp(*solution, *other.solution);
        auto cmp = [](auto &elem, auto &value) { return elem.first < value; };

        auto &oProx = other.proximity;
        auto place = std::lower_bound(oProx.begin(), oProx.end(), div, cmp);
        oProx.emplace(place, div, solution.get());

        auto &iProx = item.proximity;
        place = std::lower_bound(iProx.begin(), iProx.end(), div, cmp);
        iProx.emplace(place, div, other.solution.get());
    }

    items_.push_back(item);  // add solution

    if (size() > params.maxPopSize())
        purge(costEvaluator);
}

size_t SubPopulation::size() const { return items_.size(); }

SubPopulation::Item const &SubPopulation::operator[](size_t idx) const
{
    return items_[idx];
}

const_iter SubPopulation::cbegin() const { return items_.cbegin(); }

const_iter SubPopulation::cend() const { return items_.cend(); }

void SubPopulation::remove(iter const &iterator)
{
    for (auto &[params, solution, fitness, proximity] : items_)
        // Remove solution from other proximities.
        for (size_t idx = 0; idx != proximity.size(); ++idx)
            if (proximity[idx].second == iterator->solution.get())
            {
                proximity.erase(proximity.begin() + idx);
                break;
            }

    items_.erase(iterator);
}

void SubPopulation::purge(CostEvaluator const &costEvaluator)
{
    // First we remove duplicates. This does not rely on the fitness values.
    while (size() > params.minPopSize)
    {
        // Remove duplicates from the subpopulation (if they exist)
        auto const pred = [&](auto &item)
        {
            return !item.proximity.empty()
                   && *item.proximity[0].second == *item.solution;
        };

        auto const duplicate = std::find_if(items_.begin(), items_.end(), pred);
        if (duplicate == items_.end())  // there are no more duplicates
            break;

        remove(duplicate);
    }

    while (size() > params.minPopSize)
    {
        // Before using fitness, we must update fitness
        updateFitness(costEvaluator);
        auto const worstFitness = std::max_element(
            items_.begin(),
            items_.end(),
            [](auto const &a, auto const &b) { return a.fitness < b.fitness; });

        remove(worstFitness);
    }
}

void SubPopulation::updateFitness(CostEvaluator const &costEvaluator)
{
    if (items_.empty())
        return;

    // clang-format off
    std::vector<size_t> byCost(size());
    std::iota(byCost.begin(), byCost.end(), 0);
    std::stable_sort(
        byCost.begin(),
        byCost.end(),
        [&](size_t a, size_t b)
        {
            return costEvaluator.penalisedCost(*items_[a].solution)
                   < costEvaluator.penalisedCost(*items_[b].solution);
        });
    // clang-format on

    std::vector<std::pair<double, size_t>> diversity;
    for (size_t costRank = 0; costRank != size(); costRank++)
    {
        auto const dist = items_[byCost[costRank]].avgDistanceClosest();
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
        items_[idx].fitness = (costRank + divWeight * divRank) / (2 * popSize);
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
