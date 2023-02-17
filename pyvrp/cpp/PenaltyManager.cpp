#include "PenaltyManager.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

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

void PenaltyManager::updateCapacityPenalty(double currFeasPct)
{
    capacityPenalty = compute(capacityPenalty, currFeasPct);
}

void PenaltyManager::updateTimeWarpPenalty(double currFeasPct)
{
    timeWarpPenalty = compute(timeWarpPenalty, currFeasPct);
}

PenaltyManager::PenaltyBooster PenaltyManager::getPenaltyBooster()
{
    return PenaltyBooster(*this);
}
