#ifndef CONFIG_H
#define CONFIG_H

#include <climits>
#include <iosfwd>
#include <string>
#include <utility>

// Stores all the parameters values
struct Config
{
    int seed = 0;                    // Random seed
    size_t nbIter = 10'000;          // iters without improvement
    int timeLimit = INT_MAX;         // time limit in seconds
    bool collectStatistics = false;  // collect runtime statistics?

    size_t initialTimeWarpPenalty = 6;

    size_t nbPenaltyManagement = 47;  // manage penalties every # iterations
    double feasBooster = 2.5;         // special increase penalty if no feas
    double penaltyIncrease = 1.34;    // regular increase if below target feas
    double penaltyDecrease = 0.32;    // regular decrease if above target feas

    size_t minPopSize = 25;
    size_t generationSize = 40;    // max size before culling a generation
    size_t nbElite = 4;            // number of elite individuals in pop
    double lbDiversity = 0.1;      // minimum pct difference in parent diversity
    double ubDiversity = 0.5;      // maximum pct difference in parent diversity
    size_t nbClose = 5;            // # individuals when calculating diversity
    double targetFeasible = 0.43;  // target feasible pop fraction
    size_t nbKeepOnRestart = 0;    // # individuals to keep when restarting

    size_t repairProbability = 79;  // probability of repair if infeasible
    size_t repairBooster = 12;      // penalty booster when repairing

    size_t selectProbability = 90;  // offspring selection probability

    int nbVeh = INT_MAX;  // Number of vehicles

    // Granular search parameter, limits the number of moves in the RI local
    // search
    size_t nbGranular = 34;

    int weightWaitTime = 18;  // weight for wait-time nearness
    int weightTimeWarp = 20;  // weight for time warp nearness

    bool shouldIntensify = true;  // try to further improve a new best solution?

    // Number of nodes we improve by enumeration in LS postprocessing
    size_t postProcessPathLength = 7;

    explicit Config(int seed = 0,
                    size_t nbIter = 10'000,
                    int timeLimit = INT_MAX,
                    bool collectStatistics = false,
                    size_t initialTimeWarpPenalty = 6,
                    size_t nbPenaltyManagement = 47,
                    double feasBooster = 2.5,
                    double penaltyIncrease = 1.34,
                    double penaltyDecrease = 0.32,
                    size_t minPopSize = 25,
                    size_t generationSize = 40,
                    size_t nbElite = 4,
                    double lbDiversity = 0.1,
                    double ubDiversity = 0.5,
                    size_t nbClose = 5,
                    double targetFeasible = 0.43,
                    size_t nbKeepOnRestart = 0,
                    size_t repairProbability = 79,
                    size_t repairBooster = 12,
                    size_t selectProbability = 90,
                    int nbVeh = INT_MAX,
                    size_t nbGranular = 34,
                    int weightWaitTime = 18,
                    int weightTimeWarp = 20,
                    bool shouldIntensify = true,
                    size_t postProcessPathLength = 7)
        : seed(seed),
          nbIter(nbIter),
          timeLimit(timeLimit),
          collectStatistics(collectStatistics),
          initialTimeWarpPenalty(initialTimeWarpPenalty),
          nbPenaltyManagement(nbPenaltyManagement),
          feasBooster(feasBooster),
          penaltyIncrease(penaltyIncrease),
          penaltyDecrease(penaltyDecrease),
          minPopSize(minPopSize),
          generationSize(generationSize),
          nbElite(nbElite),
          lbDiversity(lbDiversity),
          ubDiversity(ubDiversity),
          nbClose(nbClose),
          targetFeasible(targetFeasible),
          nbKeepOnRestart(nbKeepOnRestart),
          repairProbability(repairProbability),
          repairBooster(repairBooster),
          selectProbability(selectProbability),
          nbVeh(nbVeh),
          nbGranular(nbGranular),
          weightWaitTime(weightWaitTime),
          weightTimeWarp(weightTimeWarp),
          shouldIntensify(shouldIntensify),
          postProcessPathLength(postProcessPathLength)
    {
    }
};

#endif  // CONFIG_H
