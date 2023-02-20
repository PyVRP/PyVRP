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
    loadFeas.emplace_back(isLoadFeasible);

    if (loadFeas.size() == params.numRegistrationsBetweenPenaltyUpdates)
    {
        double const sum = std::accumulate(loadFeas.begin(), loadFeas.end(), 0);
        auto const avg = loadFeas.empty() ? 1.0 : sum / loadFeas.size();

        capacityPenalty = compute(capacityPenalty, avg);
        loadFeas.clear();
    }
}

void PenaltyManager::registerTimeFeasible(bool isTimeFeasible)
{
    timeFeas.emplace_back(isTimeFeasible);

    if (timeFeas.size() == params.numRegistrationsBetweenPenaltyUpdates)
    {
        double const sum = std::accumulate(timeFeas.begin(), timeFeas.end(), 0);
        auto const avg = timeFeas.empty() ? 1.0 : sum / timeFeas.size();

        timeWarpPenalty = compute(timeWarpPenalty, avg);
        timeFeas.clear();
    }
}

PenaltyManager::PenaltyBooster PenaltyManager::getPenaltyBooster()
{
    return PenaltyBooster(*this);
}
