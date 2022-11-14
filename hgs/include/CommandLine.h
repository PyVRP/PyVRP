#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include "Config.h"

#include <iostream>  // needed for parse() below
#include <string>

// Class interacting with the command line
// Parameters can be read from the command line and information can be written
// to the command line
class CommandLine
{
    int argc;
    char **argv;

public:
    // argc is the number of command line arguments
    // argv are the command line arguments:
    //		0) The path to the genvrp executable to run
    //		1) The path to the instance to consider
    //		2) The path to the file where the solution will be stored
    //      .) Possibly combinations of command line argument descriptions with
    //         their value (counted as 2 per argument in argc)
    CommandLine(int argc, char **argv) : argc(argc), argv(argv)
    {
        // Check if the number of arguments is odd and at least three, since
        // the two paths (+program name) should at least be given
        if (argc % 2 != 1 || argc < 3)
            throw std::invalid_argument("Incorrect number of arguments");
    }

    [[nodiscard]] char const *instPath() const { return argv[1]; }

    [[nodiscard]] char const *solPath() const { return argv[2]; }

    // Extracts run configurations from command line arguments
    Config parse()
    {
        Config config;

        // Go over all possible command line arguments and store their
        // values. Explanations per command line argument can be found at
        // their variable declaration.
        for (int i = 3; i < argc; i += 2)
        {
            if (std::string(argv[i]) == "-t")
                config.timeLimit = atoi(argv[i + 1]);
            else if (std::string(argv[i]) == "-it")
                config.nbIter = static_cast<size_t>(atoi(argv[i + 1]));
            else if (std::string(argv[i]) == "-seed")
                config.seed = atoi(argv[i + 1]);
            else if (std::string(argv[i]) == "-veh")
                config.nbVeh = atoi(argv[i + 1]);
            else if (std::string(argv[i]) == "-collectStatistics")
                config.collectStatistics = atoi(argv[i + 1]) != 0;
            else if (std::string(argv[i]) == "-nbGranular")
                config.nbGranular = static_cast<size_t>(atoi(argv[i + 1]));
            else if (std::string(argv[i]) == "-initialTimeWarpPenalty")
                config.initialTimeWarpPenalty
                    = static_cast<size_t>(atoi(argv[i + 1]));
            else if (std::string(argv[i]) == "-feasBooster")
                config.feasBooster = atof(argv[i + 1]);
            else if (std::string(argv[i]) == "-minPopSize")
                config.minPopSize = static_cast<size_t>(atoi(argv[i + 1]));
            else if (std::string(argv[i]) == "-generationSize")
                config.generationSize = static_cast<size_t>(atoi(argv[i + 1]));
            else if (std::string(argv[i]) == "-nbElite")
                config.nbElite = static_cast<size_t>(atoi(argv[i + 1]));
            else if (std::string(argv[i]) == "-nbClose")
                config.nbClose = static_cast<size_t>(atoi(argv[i + 1]));
            else if (std::string(argv[i]) == "-targetFeasible")
                config.targetFeasible = atof(argv[i + 1]);
            else if (std::string(argv[i]) == "-repairProbability")
                config.repairProbability
                    = static_cast<size_t>(atoi(argv[i + 1]));
            else if (std::string(argv[i]) == "-repairBooster")
                config.repairBooster = static_cast<size_t>(atoi(argv[i + 1]));
        }

        return config;
    }
};

#endif
