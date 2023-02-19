#include "PenaltyManager.h"

#include <numeric>

PenaltyManager::PenaltyManager(unsigned int vehicleCapacity,
                               PenaltyParams params)
    : params(params),
      vehicleCapacity(vehicleCapacity),
      capacityPenalty(params.initCapacityPenalty),
      timeWarpPenalty(params.initTimeWarpPenalty)
{
}

unsigned int PenaltyManager::compute(unsigned int penalty, double feasPct) const
{
    auto const diff = params.targetFeasible - feasPct;

    if (-0.05 < diff && diff < 0.05)  // allow some margins on the difference
        return penalty;               // between target and actual

    auto dPenalty = static_cast<double>(penalty);

    // +- 1 to ensure we do not get stuck at the same integer values, bounded
    // to [1, 1000] to avoid overflow in cost computations.
    if (diff > 0)
        dPenalty = std::min(params.penaltyIncrease * dPenalty + 1, 1000.);
    else
        dPenalty = std::max(params.penaltyDecrease * dPenalty - 1, 1.);

    return static_cast<int>(dPenalty);
}

void PenaltyManager::registerLoadFeasible(bool isLoadFeasible)
{
    loadFeasible.emplace_back(isLoadFeasible);

    if (loadFeasible.size() == params.numRegistrationsBetweenPenaltyUpdates)
    {
        auto const sum
            = std::accumulate(loadFeasible.begin(), loadFeasible.end(), 0.);
        auto const avg = loadFeasible.empty() ? 1.0 : sum / loadFeasible.size();

        capacityPenalty = compute(capacityPenalty, avg);
        loadFeasible.clear();
    }
}

void PenaltyManager::registerTimeWarpFeasible(bool isTimeWarpFeasible)
{
    twFeasible.emplace_back(isTimeWarpFeasible);

    if (twFeasible.size() == params.numRegistrationsBetweenPenaltyUpdates)
    {
        auto const sum
            = std::accumulate(twFeasible.begin(), twFeasible.end(), 0.);
        auto const avg = twFeasible.empty() ? 1.0 : sum / twFeasible.size();

        timeWarpPenalty = compute(timeWarpPenalty, avg);
        twFeasible.clear();
    }
}

unsigned int PenaltyManager::loadPenalty(unsigned int load) const
{
    if (load > vehicleCapacity)
        return (load - vehicleCapacity) * capacityPenalty;

    return 0;
}

unsigned int PenaltyManager::twPenalty(unsigned int timeWarp) const
{
    return timeWarp * timeWarpPenalty;
}

PenaltyManager::PenaltyBooster PenaltyManager::getPenaltyBooster()
{
    return PenaltyBooster(*this);
}
