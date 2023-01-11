#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include "LocalSearchParams.h"
#include "PenaltyParams.h"
#include "PopulationParams.h"
#include "SolverParams.h"

#include <climits>
#include <sstream>
#include <stdexcept>
#include <string>

/**
 * The CommandLine class takes the argc and argv parameters and parses them
 * for algorithm configuration settings, as well as the instance (input data)
 * and solution (output result) paths.
 */
class CommandLine
{
    struct Config
    {
        int seed;                // Random seed
        int timeLimit;           // time limit in seconds
        bool collectStatistics;  // collect runtime statistics?

        size_t initialCapacityPenalty;  // initial load violation penalty
        size_t initialTimeWarpPenalty;  // initial time warp penalty

        size_t nbPenaltyManagement;  // manage penalties every # iterations
        double penaltyIncrease;      // regular increase if below target feas
        double penaltyDecrease;      // regular decrease if above target feas

        size_t minPopSize;      // minimum population size
        size_t generationSize;  // max size before culling a generation
        size_t nbElite;         // number of elite individuals in pop
        double lbDiversity;     // minimum pct difference in parent diversity
        double ubDiversity;     // maximum pct difference in parent diversity
        size_t nbClose;         // # individuals when calculating diversity
        double targetFeasible;  // target feasible pop fraction

        size_t repairProbability;  // probability of repair if infeasible
        size_t repairBooster;      // penalty booster when repairing

        size_t nbGranular;  // granular neighbourhood size

        int weightWaitTime;  // weight for wait-time nearness
        int weightTimeWarp;  // weight for time warp nearness

        bool shouldIntensify;  // try to further improve a new best solution?

        // Number of nodes we improve by enumeration in LS postprocessing
        size_t postProcessPathLength;

        explicit Config(int seed = 0,
                        int timeLimit = INT_MAX,
                        bool collectStatistics = false,
                        size_t initialCapacityPenalty = 20,
                        size_t initialTimeWarpPenalty = 6,
                        size_t nbPenaltyManagement = 47,
                        double penaltyIncrease = 1.34,
                        double penaltyDecrease = 0.32,
                        size_t minPopSize = 25,
                        size_t generationSize = 40,
                        size_t nbElite = 4,
                        double lbDiversity = 0.1,
                        double ubDiversity = 0.5,
                        size_t nbClose = 5,
                        double targetFeasible = 0.43,
                        size_t repairProbability = 79,
                        size_t repairBooster = 12,
                        size_t nbGranular = 34,
                        int weightWaitTime = 18,
                        int weightTimeWarp = 20,
                        bool shouldIntensify = true,
                        size_t postProcessPathLength = 7)
            : seed(seed),
              timeLimit(timeLimit),
              collectStatistics(collectStatistics),
              initialCapacityPenalty(initialCapacityPenalty),
              initialTimeWarpPenalty(initialTimeWarpPenalty),
              nbPenaltyManagement(nbPenaltyManagement),
              penaltyIncrease(penaltyIncrease),
              penaltyDecrease(penaltyDecrease),
              minPopSize(minPopSize),
              generationSize(generationSize),
              nbElite(nbElite),
              lbDiversity(lbDiversity),
              ubDiversity(ubDiversity),
              nbClose(nbClose),
              targetFeasible(targetFeasible),
              repairProbability(repairProbability),
              repairBooster(repairBooster),
              nbGranular(nbGranular),
              weightWaitTime(weightWaitTime),
              weightTimeWarp(weightTimeWarp),
              shouldIntensify(shouldIntensify),
              postProcessPathLength(postProcessPathLength)
        {
        }
    };

    Config config;
    int argc;
    char **argv;

    /**
     * Extracts algorithm configuration.
     */
    void parse()
    {
        for (auto idx = 3; idx != argc; idx += 2)
        {
            std::istringstream stream(argv[idx + 1]);

            if (std::string(argv[idx]) == "--seed")
                stream >> config.seed;
            else if (std::string(argv[idx]) == "--timeLimit")
                stream >> config.timeLimit;
            else if (std::string(argv[idx]) == "--collectStatistics")
                stream >> std::boolalpha >> config.collectStatistics;
            else if (std::string(argv[idx]) == "--initialTimeWarpPenalty")
                stream >> config.initialTimeWarpPenalty;
            else if (std::string(argv[idx]) == "--nbPenaltyManagement")
                stream >> config.nbPenaltyManagement;
            else if (std::string(argv[idx]) == "--penaltyIncrease")
                stream >> config.penaltyIncrease;
            else if (std::string(argv[idx]) == "--penaltyDecrease")
                stream >> config.penaltyDecrease;
            else if (std::string(argv[idx]) == "--minPopSize")
                stream >> config.minPopSize;
            else if (std::string(argv[idx]) == "--generationSize")
                stream >> config.generationSize;
            else if (std::string(argv[idx]) == "--nbElite")
                stream >> config.nbElite;
            else if (std::string(argv[idx]) == "--lbDiversity")
                stream >> config.lbDiversity;
            else if (std::string(argv[idx]) == "--ubDiversity")
                stream >> config.ubDiversity;
            else if (std::string(argv[idx]) == "--nbClose")
                stream >> config.nbClose;
            else if (std::string(argv[idx]) == "--targetFeasible")
                stream >> config.targetFeasible;
            else if (std::string(argv[idx]) == "--repairProbability")
                stream >> config.repairProbability;
            else if (std::string(argv[idx]) == "--repairBooster")
                stream >> config.repairBooster;
            else if (std::string(argv[idx]) == "--nbGranular")
                stream >> config.nbGranular;
            else if (std::string(argv[idx]) == "--weightWaitTime")
                stream >> config.weightWaitTime;
            else if (std::string(argv[idx]) == "--weightTimeWarp")
                stream >> config.weightTimeWarp;
            else if (std::string(argv[idx]) == "--shouldIntensify")
                stream >> std::boolalpha >> config.shouldIntensify;
            else if (std::string(argv[idx]) == "--postProcessPathLength")
                stream >> config.postProcessPathLength;

            if (stream.fail())
            {
                std::ostringstream msg;
                // clang-format off
                msg << "Invalid argument: '"
                    << argv[idx]
                    << "' cannot be '"
                    << argv[idx + 1]
                    << "'.";
                // clang-format on

                throw std::invalid_argument(msg.str());
            }
        }
    }

public:
    // argc is the number of command line arguments
    // argv are the command line arguments:
    //		0) The path to the genvrp executable to run
    //		1) The path to the instance to consider
    //		2) The path to the file where the solution will be stored
    //      .) Possibly combinations of command line argument descriptions with
    //         their value (counted as 2 per argument in argc)
    CommandLine(int argc, char **argv) : config(), argc(argc), argv(argv)
    {
        // Check if the number of arguments is odd and at least three, since
        // the two paths and program name should at least be given.
        if (argc % 2 != 1 || argc < 3)
            throw std::invalid_argument("Incorrect number of arguments");

        parse();
    }

    [[nodiscard]] Config const &getConfig() const { return config; }

    [[nodiscard]] LocalSearchParams localSearchParams() const
    {
        return {static_cast<size_t>(config.weightWaitTime),
                static_cast<size_t>(config.weightTimeWarp),
                config.nbGranular,
                config.postProcessPathLength};
    }

    [[nodiscard]] PenaltyParams penaltyParams() const
    {
        return {static_cast<unsigned int>(config.initialCapacityPenalty),
                static_cast<unsigned int>(config.initialTimeWarpPenalty),
                static_cast<unsigned int>(config.repairBooster),
                config.penaltyIncrease,
                config.penaltyDecrease,
                config.targetFeasible};
    }

    [[nodiscard]] PopulationParams populationParams() const
    {
        return {config.minPopSize,
                config.generationSize,
                config.nbElite,
                config.nbClose,
                config.lbDiversity,
                config.ubDiversity};
    }

    [[nodiscard]] SolverParams solverParams() const
    {
        return {config.nbPenaltyManagement,
                config.repairProbability,
                config.collectStatistics,
                config.shouldIntensify};
    }

    [[nodiscard]] char const *instPath() const { return argv[1]; }

    [[nodiscard]] char const *solPath() const { return argv[2]; }
};

#endif
