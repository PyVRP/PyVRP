#ifndef HGS_COSTEVALUATOR_H
#define HGS_COSTEVALUATOR_H

#include "Individual.h"
#include "Measure.h"

/**
 * Cost evaluator class that computes penalty values for timewarp and load.
 */
class CostEvaluator
{
    Cost capacityPenalty;
    Cost timeWarpPenalty;

public:
    CostEvaluator(unsigned int capacityPenalty, unsigned int timeWarpPenalty);

    /**
     * Computes the total excess capacity penalty for the given vehicle load.
     */
    [[nodiscard]] inline Cost loadPenalty(Load load,
                                          Load vehicleCapacity) const;

    /**
     * Computes the excess capacity penalty for the given excess load, that is,
     * the part of the load that exceeds the vehicle capacity.
     */
    [[nodiscard]] inline Cost loadPenaltyExcess(Load excessLoad) const;

    /**
     * Computes the time warp penalty for the given time warp.
     */
    [[nodiscard]] inline Cost twPenalty(Duration timeWarp) const;

    /**
     * Computes a smoothed objective (penalised cost) for a given individual.
     */
    [[nodiscard]] Cost penalisedCost(Individual const &individual) const;

    /**
     * Computes the objective for a given individual. Returns the largest
     * representable cost value if the individual is infeasible.
     */
    [[nodiscard]] Cost cost(Individual const &individual) const;
};

Cost CostEvaluator::loadPenaltyExcess(Load excessLoad) const
{
    return static_cast<Cost>(excessLoad) * capacityPenalty;
}

Cost CostEvaluator::loadPenalty(Load load, Load vehicleCapacity) const
{
    // Branchless for performance: when load > capacity we return the excess
    // load penalty; else zero. Note that when load - vehicleCapacity wraps
    // around, we return zero because load > vehicleCapacity evaluates as zero
    // (so there is no issue here due to unsignedness).
    Cost penalty = loadPenaltyExcess(load - vehicleCapacity);
    return Cost(load > vehicleCapacity) * penalty;
}

Cost CostEvaluator::twPenalty(Duration timeWarp) const
{
#ifdef VRP_NO_TIME_WINDOWS
    return 0;
#else
    return static_cast<Cost>(timeWarp) * timeWarpPenalty;
#endif
}

#endif  // HGS_COSTEVALUATOR_H
