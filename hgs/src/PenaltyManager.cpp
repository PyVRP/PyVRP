#include "PenaltyManager.h"

#include <stdexcept>

PenaltyManager::PenaltyManager(unsigned int initCapacityPenalty,
                               unsigned int initTimeWarpPenalty,
                               double penaltyIncrease,
                               double penaltyDecrease,
                               double targetFeasible,
                               unsigned int vehicleCapacity,
                               unsigned int repairBooster)
    : capacityPenalty(initCapacityPenalty),
      timeWarpPenalty(initTimeWarpPenalty),
      penaltyIncrease(penaltyIncrease),
      penaltyDecrease(penaltyDecrease),
      targetFeasible(targetFeasible),
      vehicleCapacity(vehicleCapacity),
      repairBooster(repairBooster)
{
    if (penaltyIncrease < 1.)
        throw std::invalid_argument("Expected penaltyIncrease >= 1.");

    if (penaltyDecrease < 0. || penaltyDecrease > 1.)
        throw std::invalid_argument("Expected penaltyDecrease in [0, 1].");

    if (targetFeasible < 0. || targetFeasible > 1.)
        throw std::invalid_argument("Expected targetFeasible in [0, 1].");

    if (repairBooster < 1)
        throw std::invalid_argument("Expected repairBooster >= 1.");
}

unsigned int PenaltyManager::compute(unsigned int penalty, double feasPct) const
{
    auto const diff = targetFeasible - feasPct;

    if (-0.05 < diff && diff < 0.05)  // allow some margins on the difference
        return penalty;               // between target and actual

    auto dPenalty = static_cast<double>(penalty);

    // +- 1 to ensure we do not get stuck at the same integer values, bounded
    // to [1, 1000] to avoid overflow in cost computations.
    if (diff > 0)
        dPenalty = std::min(penaltyIncrease * dPenalty + 1, 1000.);
    else
        dPenalty = std::max(penaltyDecrease * dPenalty - 1, 1.);

    return static_cast<int>(dPenalty);
}

void PenaltyManager::updateCapacityPenalty(double currFeasPct)
{
    capacityPenalty = compute(capacityPenalty, currFeasPct);
}

void PenaltyManager::updateTimeWarpPenalty(double currFeasPct)
{
    timeWarpPenalty = compute(timeWarpPenalty, currFeasPct);
}

unsigned int PenaltyManager::loadPenalty(unsigned int load) const
{
    auto const excessLoad = std::max(load, vehicleCapacity) - vehicleCapacity;
    return excessLoad * capacityPenalty;
}

unsigned int PenaltyManager::twPenalty(unsigned int timeWarp) const
{
    return timeWarp * timeWarpPenalty;
}

PenaltyManager::PenaltyBooster PenaltyManager::getPenaltyBooster()
{
    return PenaltyBooster(*this);
}
