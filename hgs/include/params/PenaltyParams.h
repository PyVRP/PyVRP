#ifndef HGS_PENALTYPARAMS_H
#define HGS_PENALTYPARAMS_H

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
          targetFeasible(targetFeasible){};
};

#endif  // HGS_PENALTYPARAMS_H
