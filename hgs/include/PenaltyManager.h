#ifndef HGS_PENALTYMANAGER_H
#define HGS_PENALTYMANAGER_H

class PenaltyManager
{
    unsigned int capacityPenalty;
    unsigned int timeWarpPenalty;

    double const penaltyIncrease;
    double const penaltyDecrease;
    double const targetFeasible;
    unsigned int const vehicleCapacity;
    unsigned int const repairBooster;

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
    PenaltyManager(unsigned int initCapacityPenalty,
                   unsigned int initTimeWarpPenalty,
                   double penaltyIncrease,
                   double penaltyDecrease,
                   double targetFeasible,
                   unsigned int vehicleCapacity,
                   unsigned int repairBooster);

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
