#include "PenaltyManager.h"

#include <algorithm>
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

void PenaltyManager::updateCapacityPenalty(double currFeasPct)
{
    auto penalty = static_cast<double>(capacityPenalty);
    auto const diff = targetFeasible - currFeasPct;

    if (-0.05 < diff && diff < 0.05)  // allow some margins on the difference
        return;                       // between target and actual

    // +- 1 to ensure we do not get stuck at the same integer values,
    // bounded to [1, 1000] to avoid overflow in cost computations.
    if (diff > 0)
        penalty = std::min(penaltyIncrease * penalty + 1, 1000.);
    else
        penalty = std::max(penaltyDecrease * penalty - 1, 1.);

    capacityPenalty = static_cast<int>(penalty);
}

void PenaltyManager::updateTimeWarpPenalty(double currFeasPct)
{
    auto penalty = static_cast<double>(timeWarpPenalty);
    auto const diff = targetFeasible - currFeasPct;

    if (-0.05 < diff && diff < 0.05)  // allow some margins on the difference
        return;                       // between target and actual

    // +- 1 to ensure we do not get stuck at the same integer values,
    // bounded to [1, 1000] to avoid overflow in cost computations.
    if (diff > 0)
        penalty = std::min(penaltyIncrease * penalty + 1, 1000.);
    else
        penalty = std::max(penaltyDecrease * penalty - 1, 1.);

    timeWarpPenalty = static_cast<int>(penalty);
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
