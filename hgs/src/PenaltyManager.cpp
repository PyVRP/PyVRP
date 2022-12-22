#include "PenaltyManager.h"

#include <algorithm>

void PenaltyManager::updateCapacityPenalty(double currFeasPct)
{
    auto penalty = static_cast<double>(capacityPenalty);

    // +- 1 to ensure we do not get stuck at the same integer values,
    // bounded to [1, 1000] to avoid overflow in cost computations.
    if (currFeasPct < targetFeasible - 0.05)
        penalty = std::min(penaltyIncrease * penalty + 1, 1000.);
    else if (currFeasPct > targetFeasible + 0.05)
        penalty = std::max(penaltyDecrease * penalty - 1, 1.);

    capacityPenalty = static_cast<int>(penalty);
}

void PenaltyManager::updateTimeWarpPenalty(double currFeasPct)
{
    auto penalty = static_cast<double>(timeWarpPenalty);

    // +- 1 to ensure we do not get stuck at the same integer values,
    // bounded to [1, 1000] to avoid overflow in cost computations.
    if (currFeasPct < targetFeasible - 0.05)
        penalty = std::min(penaltyIncrease * penalty + 1, 1000.);
    else if (currFeasPct > targetFeasible + 0.05)
        penalty = std::max(penaltyDecrease * penalty - 1, 1.);

    timeWarpPenalty = static_cast<int>(penalty);
}

int PenaltyManager::loadPenalty(int load) const
{
    return std::max(load - vehicleCapacity, 0) * capacityPenalty;
}

int PenaltyManager::twPenalty(int timeWarp) const
{
    return timeWarp * timeWarpPenalty;
}

PenaltyManager::PenaltyBooster PenaltyManager::getPenaltyBooster()
{
    return PenaltyBooster(*this);
}
