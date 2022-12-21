#ifndef HGS_PENALTYMANAGER_H
#define HGS_PENALTYMANAGER_H

class PenaltyManager
{
    int capacityPenalty;
    int timeWarpPenalty;

    int const vehicleCapacity;
    int const repairBooster;

    // Penalty booster that increases the penalty on capacity and time window
    // violations during the object's lifetime.
    struct PenaltyBooster
    {
        PenaltyManager const &mngr;
        int const oldCapacityPenalty;
        int const oldTimeWarpPenalty;

        explicit PenaltyBooster(PenaltyManager const &mngr)
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
                   int vehicleCapacity,
                   int repairBooster)
        : capacityPenalty(initCapacityPenalty),
          timeWarpPenalty(initTimeWarpPenalty),
          vehicleCapacity(vehicleCapacity),
          repairBooster(repairBooster)
    {
        // TODO
    }

    void updateCapacityPenalty()
    {
        auto penalty = static_cast<double>(capacityPenalty);

        // +- 1 to ensure we do not get stuck at the same integer values,
        // bounded to [1, 1000] to avoid overflow in cost computations.
        if (currFeas < targetFeasible - 0.05)
            penalty = std::min(penaltyIncrease * penalty + 1, 1000);
        else if (currFeas > targetFeasible + 0.05)
            penalty = std::max(penaltyDecrease * penalty - 1, 1);

        capacityPenalty = static_cast<int>(penalty);
    }

    void updateTimeWarpPenalty()
    {
        auto penalty = static_cast<double>(timeWarpPenalty);

        // +- 1 to ensure we do not get stuck at the same integer values,
        // bounded to [1, 1000] to avoid overflow in cost computations.
        if (currFeas < targetFeasible - 0.05)
            penalty = std::min(penaltyIncrease * penalty + 1, 1000);
        else if (currFeas > targetFeasible + 0.05)
            penalty = std::max(penaltyDecrease * penalty - 1, 1);

        timeWarpPenalty = static_cast<int>(penalty);
    }

    /**
     * Computes the total excess capacity penalty for the given load.
     */
    [[nodiscard]] int loadPenalty(int load) const
    {
        return std::max(load - vehicleCapacity, 0) * capacityPenalty;
    }

    /**
     * Computes the total time warp penalty for the give time warp.
     */
    [[nodiscard]] int twPenalty(int timeWarp) const
    {
        return timeWarp * timeWarpPenalty;
    }

    /**
     * Returns a penalty booster that temporarily increases infeasibility
     * penalties (while the booster lives).
     */
    [[nodiscard]] PenaltyBooster getPenaltyBooster()
    {
        return PenaltyBooster(*this);
    }
};

#endif  // HGS_PENALTYMANAGER_H
