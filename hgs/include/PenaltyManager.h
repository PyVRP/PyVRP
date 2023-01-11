#ifndef HGS_PENALTYMANAGER_H
#define HGS_PENALTYMANAGER_H

#include "PenaltyParams.h"

class PenaltyManager
{
    PenaltyParams const params;
    unsigned int const vehicleCapacity;

    unsigned int capacityPenalty;
    unsigned int timeWarpPenalty;

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
            mngr.capacityPenalty *= mngr.params.repairBooster;
            mngr.timeWarpPenalty *= mngr.params.repairBooster;
        }

        ~PenaltyBooster()
        {
            mngr.capacityPenalty = oldCapacityPenalty;
            mngr.timeWarpPenalty = oldTimeWarpPenalty;
        }
    };

    // Computes and returns the new penalty value, given the current value and
    // the percentage of feasible solutions since the last update.
    [[nodiscard]] unsigned int compute(unsigned int penalty,
                                       double feasPct) const;

public:
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
    [[nodiscard]] unsigned int loadPenalty(unsigned int load) const;

    /**
     * Computes the time warp penalty for the given time warp.
     */
    [[nodiscard]] unsigned int twPenalty(unsigned int timeWarp) const;

    /**
     * Returns a penalty booster that temporarily increases infeasibility
     * penalties (while the booster lives).
     */
    [[nodiscard]] PenaltyBooster getPenaltyBooster();
};

#endif  // HGS_PENALTYMANAGER_H
