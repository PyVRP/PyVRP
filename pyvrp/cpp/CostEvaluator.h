#ifndef HGS_COSTEVALUATOR_H
#define HGS_COSTEVALUATOR_H

#include "Individual.h"
#include "Measure.h"

/**
 * Cost evaluator class that computes penalty values for timewarp and load.
 */
class CostEvaluator
{
    cost_type capacityPenalty;
    cost_type timeWarpPenalty;

public:
    CostEvaluator(unsigned int capacityPenalty, unsigned int timeWarpPenalty);

    /**
     * Computes the total excess capacity penalty for the given vehicle load.
     */
    [[nodiscard]] inline cost_type
    loadPenalty(capacity_type load, capacity_type vehicleCapacity) const;

    /**
     * Computes the excess capacity penalty for the given excess load, that is,
     * the part of the load that exceeds the vehicle capacity.
     */
    [[nodiscard]] inline cost_type
    loadPenaltyExcess(capacity_type excessLoad) const;

    /**
     * Computes the time warp penalty for the given time warp.
     */
    [[nodiscard]] inline cost_type twPenalty(Duration timeWarp) const;

    /**
     * Computes a smoothed objective (penalised cost) for a given individual.
     */
    [[nodiscard]] cost_type penalisedCost(Individual const &individual) const;

    /**
     * Computes the objective for a given individual. Returns the largest
     * representable cost value if the individual is infeasible.
     */
    [[nodiscard]] cost_type cost(Individual const &individual) const;
};

cost_type CostEvaluator::loadPenaltyExcess(capacity_type excessLoad) const
{
    return static_cast<cost_type>(excessLoad) * capacityPenalty;
}

cost_type CostEvaluator::loadPenalty(capacity_type load,
                                     capacity_type vehicleCapacity) const
{
    // Branchless for performance: when load > capacity we return the excess
    // load penalty; else zero. Note that when load - vehicleCapacity wraps
    // around, we return zero because load > vehicleCapacity evaluates as zero
    // (so there is no issue here due to unsignedness).
    cost_type penalty = loadPenaltyExcess(load - vehicleCapacity);
    return cost_type(load > vehicleCapacity) * penalty;
}

cost_type CostEvaluator::twPenalty(Duration timeWarp) const
{
#ifdef VRP_NO_TIME_WINDOWS
    return 0;
#else
    return static_cast<cost_type>(timeWarp) * timeWarpPenalty;
#endif
}

#endif  // HGS_COSTEVALUATOR_H
