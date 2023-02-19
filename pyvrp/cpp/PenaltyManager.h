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
                  unsigned int numRegistrationsBetweenPenaltyUpdates = 47,
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
 * Dynamic penalty manager class. This class manages current time warp and
 * load penalties, and can be asked to provide those values. Additionally, it
 * updates these penalties based on recent history, and can be used to provide
 * a temporary penalty booster object that increases the penalties.
 */
class PenaltyManager
{
    PenaltyParams const params;
    unsigned int const vehicleCapacity;

    unsigned int capacityPenalty;
    unsigned int timeWarpPenalty;

    std::vector<bool> loadFeasible;
    std::vector<bool> twFeasible;

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
     * is updated if sufficiently many results have been gathered.
     */
    void registerLoadFeasible(bool isLoadFeasible);

    /**
     * Registers another time window feasibility result. The current time warp
     * penalty is updated if sufficiently many results have been gathered.
     */
    void registerTimeWarpFeasible(bool isTimeWarpFeasible);

    /**
     * Computes the total excess capacity penalty for the given vehicle load.
     *
     * TODO maybe inline
     */
    [[nodiscard]] unsigned int loadPenalty(unsigned int load) const;

    /**
     * Computes the time warp penalty for the given time warp.
     *
     * TODO maybe inline
     */
    [[nodiscard]] unsigned int twPenalty(unsigned int timeWarp) const;

    /**
     * Returns a penalty booster that temporarily increases infeasibility
     * penalties (while the booster lives).
     */
    [[nodiscard]] PenaltyBooster getPenaltyBooster();
};

#endif  // HGS_PENALTYMANAGER_H
