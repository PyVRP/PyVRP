#ifndef PYVRP_COSTEVALUATOR_H
#define PYVRP_COSTEVALUATOR_H

#include "Measure.h"
#include "Solution.h"

/**
 * Cost evaluator class that computes penalty values for timewarp and load.
 */
class CostEvaluator
{
    Cost weightCapacityPenalty;
    Cost volumeCapacityPenalty;
    Cost timeWarpPenalty;

public:
    CostEvaluator(Cost weightCapacityPenalty, Cost volumeCapacityPenalty, Cost timeWarpPenalty);

    /**
     * Computes the total excess weight penalty for the given vehicle load.
     */
    [[nodiscard]] inline Cost weightPenalty(Load weight, Load weightCapacity) const;

    /**
     * Computes the excess weight penalty for the given excess load, that is,
     * the part of the load that exceeds the vehicle weight.
     */
    [[nodiscard]] inline Cost weightPenaltyExcess(Load excessWeight) const;


    /**
     * Computes the total excess volume penalty for the given vehicle load.
     */
    [[nodiscard]] inline Cost volumePenalty(Load volume, Load volumeCapacity) const;

    /**
     * Computes the excess volume penalty for the given excess load, that is,
     * the part of the load that exceeds the vehicle volume.
     */
    [[nodiscard]] inline Cost volumePenaltyExcess(Load excessVolume) const;

    /**
     * Computes the time warp penalty for the given time warp.
     */
    [[nodiscard]] inline Cost twPenalty(Duration timeWarp) const;

    /**
     * Computes a smoothed objective (penalised cost) for a given solution.
     */
    [[nodiscard]] Cost penalisedCost(Solution const &solution) const;

    /**
     * Computes the objective for a given solution. Returns the largest
     * representable cost value if the solution is infeasible.
     */
    [[nodiscard]] Cost cost(Solution const &solution) const;
};

Cost CostEvaluator::weightPenaltyExcess(Load excessWeight) const
{
    return static_cast<Cost>(excessWeight) * weightCapacityPenalty;
}

Cost CostEvaluator::weightPenalty(Load weight, Load weightCapacity) const
{
    // Branchless for performance: when weight > weightCapacity we return the excess
    // weight penalty; else zero. Note that when weight - weightCapacity wraps
    // around, we return zero because weight > weightCapacity evaluates as zero
    // (so there is no issue here due to unsignedness).
    Cost penalty = weightPenaltyExcess(weight - weightCapacity);
    return Cost(weight > weightCapacity) * penalty;
}


Cost CostEvaluator::volumePenaltyExcess(Load excessVolume) const
{
    return static_cast<Cost>(excessVolume) * volumeCapacityPenalty;
}

Cost CostEvaluator::volumePenalty(Load volume, Load volumeCapacity) const
{
    // Branchless for performance: when volume > volumeCapacity we return the excess
    // volume penalty; else zero. Note that when volume - volumeCapacity wraps
    // around, we return zero because volume > volumeCapacity evaluates as zero
    // (so there is no issue here due to unsignedness).
    Cost penalty = volumePenaltyExcess(volume - volumeCapacity);
    return Cost(volume > volumeCapacity) * penalty;
}

Cost CostEvaluator::twPenalty([[maybe_unused]] Duration timeWarp) const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return 0;
#else
    return static_cast<Cost>(timeWarp) * timeWarpPenalty;
#endif
}

#endif  // PYVRP_COSTEVALUATOR_H
