#ifndef HGS_PENALTYMANAGER_H
#define HGS_PENALTYMANAGER_H

#include <stdexcept>

struct PenaltyParams
{
    unsigned int const initCapacityPenalty;
    unsigned int const initTimeWarpPenalty;
    unsigned int const repairBooster;

    double const penaltyIncrease;
    double const penaltyDecrease;
    double const targetFeasible;

    PenaltyParams(unsigned int initCapacityPenalty = 20,
                  unsigned int initTimeWarpPenalty = 6,
                  unsigned int repairBooster = 12,
                  double penaltyIncrease = 1.34,
                  double penaltyDecrease = 0.32,
                  double targetFeasible = 0.43)
        : initCapacityPenalty(initCapacityPenalty),
          initTimeWarpPenalty(initTimeWarpPenalty),
          repairBooster(repairBooster),
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

class PenaltyManager
{
    PenaltyParams const params;
    unsigned int const vehicleCapacity;

    unsigned int capacityPenalty;
    unsigned int timeWarpPenalty;

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
     * Updates the capacity penalty based on the percentage of load-feasible
     * solutions since the last update.
     *
     * @param currFeasPct Percentage of load feasible solutions. Should be a
     *                    number in [0, 1].
     */
    void updateCapacityPenalty(double currFeasPct);

    /**
     * Updates the time warp penalty based on the percentage of time-feasible
     * solutions since the last update.
     *
     * @param currFeasPct Percentage of time feasible solutions. Should be a
     *                    number in [0, 1].
     */
    void updateTimeWarpPenalty(double currFeasPct);

    /**
     * Computes the total excess capacity penalty for the given vehicle load.
     */
    [[nodiscard]] inline unsigned int loadPenalty(unsigned int load) const;

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

unsigned int PenaltyManager::loadPenalty(unsigned int load) const
{
    if (load > vehicleCapacity)
        return (load - vehicleCapacity) * capacityPenalty;

    return 0;
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
