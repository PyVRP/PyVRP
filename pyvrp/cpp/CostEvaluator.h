#ifndef HGS_COSTEVALUATOR_H
#define HGS_COSTEVALUATOR_H

#include "Individual.h"

/**
 * Cost evaluator class that computes penalty values for timewarp and load.
 */
class CostEvaluator
{
    unsigned int capacityPenalty;
    unsigned int timeWarpPenalty;

public:
    CostEvaluator(unsigned int capacityPenalty, unsigned int timeWarpPenalty);

    /**
     * Computes the total excess capacity penalty for the given vehicle load.
     */
    [[nodiscard]] inline unsigned int
    loadPenalty(unsigned int load, unsigned int vehicleCapacity) const;

    /**
     * Computes the excess capacity penalty for the given excess load, that is,
     * the part of the load that exceeds the vehicle capacity.
     */
    [[nodiscard]] inline unsigned int
    loadPenaltyExcess(unsigned int excessLoad) const;

    /**
     * Computes the time warp penalty for the given time warp.
     */
    [[nodiscard]] inline unsigned int twPenalty(unsigned int timeWarp) const;

    /**
     * Computes a smoothed objective (penalised cost) for a given individual.
     */
    [[nodiscard]] unsigned int
    penalisedCost(Individual const &individual) const;

    /**
     * Computes the objective for a given individual. Returns the largest
     * representable cost value if the individual is infeasible.
     */
    [[nodiscard]] unsigned int cost(Individual const &individual) const;
};

unsigned int CostEvaluator::loadPenaltyExcess(unsigned int excessLoad) const
{
    return excessLoad * capacityPenalty;
}

unsigned int CostEvaluator::loadPenalty(unsigned int load,
                                        unsigned int vehicleCapacity) const
{
    // Branchless for performance: when load > capacity we return the excess
    // load penalty; else zero. Note that when load - vehicleCapacity wraps
    // around, we return zero because load > vehicleCapacity evaluates as zero
    // (so there is no issue here due to unsignedness).
    return (load > vehicleCapacity) * loadPenaltyExcess(load - vehicleCapacity);
}

unsigned int CostEvaluator::twPenalty(unsigned int timeWarp) const
{
#ifdef VRP_NO_TIME_WINDOWS
    return 0;
#else
    return timeWarp * timeWarpPenalty;
#endif
}

#endif  // HGS_COSTEVALUATOR_H
