#ifndef HGS_PENALTYMANAGER_H
#define HGS_PENALTYMANAGER_H

#include <stdexcept>
#include <vector>

struct PenaltyParams
{
    unsigned int const initCapacityPenalty;
    unsigned int const initTimeWarpPenalty;
    unsigned int const repairBooster;

    unsigned int const numRegistrationsBetweenPenaltyUpdates;

    double const penaltyIncrease;
    double const penaltyDecrease;
    double const targetFeasible;

    PenaltyParams(unsigned int initCapacityPenalty = 20,
                  unsigned int initTimeWarpPenalty = 6,
                  unsigned int repairBooster = 12,
                  unsigned int numRegistrationsBetweenPenaltyUpdates = 50,
                  double penaltyIncrease = 1.34,
                  double penaltyDecrease = 0.32,
                  double targetFeasible = 0.43)
        : initCapacityPenalty(initCapacityPenalty),
          initTimeWarpPenalty(initTimeWarpPenalty),
          repairBooster(repairBooster),
          numRegistrationsBetweenPenaltyUpdates(
              numRegistrationsBetweenPenaltyUpdates),
          penaltyIncrease(penaltyIncrease),
          penaltyDecrease(penaltyDecrease),
          targetFeasible(targetFeasible)
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
};

/**
 * Penalty manager class. This class manages time warp and load penalties, and
 * provides penalty terms for given time warp and load values. It updates these
 * penalties based on recent history, and can be used to provide a temporary
 * penalty booster object that increases the penalties for a short duration.
 */
class PenaltyManager
{
    PenaltyParams const params;
    unsigned int const vehicleCapacity;

    unsigned int capacityPenalty;
    unsigned int timeWarpPenalty;

    std::vector<bool> loadFeas;  // tracks recent load feasibility results
    std::vector<bool> timeFeas;  // tracks recent time feasibility results

    // Computes and returns the new penalty value, given the current value and
    // the percentage of feasible solutions since the last update.
    [[nodiscard]] unsigned int compute(unsigned int penalty,
                                       double feasPct) const;

public:
    // Penalty booster that increases the penalty on capacity and time window
    // violations during the object's lifetime.
    struct PenaltyBooster
    {
        PenaltyManager &mngr;
        unsigned int const oldCapacityPenalty;
        unsigned int const oldTimeWarpPenalty;

        explicit PenaltyBooster(PenaltyManager &mngr)
            : mngr(mngr),
              oldCapacityPenalty(mngr.capacityPenalty),
              oldTimeWarpPenalty(mngr.timeWarpPenalty)
        {
        }

        void enter()
        {
            mngr.capacityPenalty *= mngr.params.repairBooster;
            mngr.timeWarpPenalty *= mngr.params.repairBooster;
        }

        void exit()
        {
            mngr.capacityPenalty = oldCapacityPenalty;
            mngr.timeWarpPenalty = oldTimeWarpPenalty;
        }
    };

    explicit PenaltyManager(unsigned int vehicleCapacity,
                            PenaltyParams params = PenaltyParams());

    /**
     * Registers another capacity feasibility result. The current load penalty
     * is updated once sufficiently many results have been gathered.
     */
    void registerLoadFeasible(bool isLoadFeasible);

    /**
     * Registers another time feasibility result. The current time warp penalty
     * is updated once sufficiently many results have been gathered.
     */
    void registerTimeFeasible(bool isTimeFeasible);

    /**
     * Computes the total excess capacity penalty for the given vehicle load.
     */
    [[nodiscard]] inline unsigned int loadPenalty(unsigned int load) const;

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
     * Returns a penalty booster that temporarily increases infeasibility
     * penalties (while the booster lives).
     */
    [[nodiscard]] PenaltyBooster getPenaltyBooster();
};

unsigned int PenaltyManager::loadPenaltyExcess(unsigned int excessLoad) const
{
    return excessLoad * capacityPenalty;
}

unsigned int PenaltyManager::loadPenalty(unsigned int load) const
{
    // Branchless for performance: when load > capacity we return the excess
    // load penalty; else zero. Note that when load - vehicleCapacity wraps
    // around, we return zero because load > vehicleCapacity evaluates as zero
    // (so there is no issue here due to unsignedness).
    return (load > vehicleCapacity) * loadPenaltyExcess(load - vehicleCapacity);
}

unsigned int PenaltyManager::twPenalty(unsigned int timeWarp) const
{
#ifdef VRP_NO_TIME_WINDOWS
    return 0;
#else
    return timeWarp * timeWarpPenalty;
#endif
}

#endif  // HGS_PENALTYMANAGER_H
