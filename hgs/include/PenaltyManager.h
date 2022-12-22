#ifndef HGS_PENALTYMANAGER_H
#define HGS_PENALTYMANAGER_H

class PenaltyManager
{
    int capacityPenalty;
    int timeWarpPenalty;

    double const penaltyIncrease;
    double const penaltyDecrease;
    double const targetFeasible;
    int const vehicleCapacity;
    int const repairBooster;

    // Penalty booster that increases the penalty on capacity and time window
    // violations during the object's lifetime.
    struct PenaltyBooster
    {
        PenaltyManager &mngr;
        int const oldCapacityPenalty;
        int const oldTimeWarpPenalty;

        explicit PenaltyBooster(PenaltyManager &mngr)
            : mngr(mngr),
              oldCapacityPenalty(mngr.capacityPenalty),
              oldTimeWarpPenalty(mngr.timeWarpPenalty)
        {
            mngr.capacityPenalty *= mngr.repairBooster;
            mngr.timeWarpPenalty *= mngr.repairBooster;
        }

        ~PenaltyBooster()
        {
            mngr.capacityPenalty = oldCapacityPenalty;
            mngr.timeWarpPenalty = oldTimeWarpPenalty;
        }
    };

public:
    PenaltyManager(int initCapacityPenalty,
                   int initTimeWarpPenalty,
                   double penaltyIncrease,
                   double penaltyDecrease,
                   double targetFeasible,
                   int vehicleCapacity,
                   int repairBooster)
        : capacityPenalty(initCapacityPenalty),
          timeWarpPenalty(initTimeWarpPenalty),
          penaltyIncrease(penaltyIncrease),
          penaltyDecrease(penaltyDecrease),
          targetFeasible(targetFeasible),
          vehicleCapacity(vehicleCapacity),
          repairBooster(repairBooster)
    {
    }

    /**
     * TODO
     *
     * @param currFeasPct
     */
    void updateCapacityPenalty(double currFeasPct);

    /**
     * TODO
     *
     * @param currFeasPct
     */
    void updateTimeWarpPenalty(double currFeasPct);

    /**
     * Computes the total excess capacity penalty for the given vehicle load.
     */
    [[nodiscard]] int loadPenalty(int load) const;

    /**
     * Computes the time warp penalty for the given time warp.
     */
    [[nodiscard]] int twPenalty(int timeWarp) const;

    /**
     * Returns a penalty booster that temporarily increases infeasibility
     * penalties (while the booster lives).
     */
    [[nodiscard]] PenaltyBooster getPenaltyBooster();
};

#endif  // HGS_PENALTYMANAGER_H
