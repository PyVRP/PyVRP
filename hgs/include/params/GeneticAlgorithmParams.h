#ifndef HGS_GENETICALGORITHMPARAMS_H
#define HGS_GENETICALGORITHMPARAMS_H

#include <iosfwd>

struct GeneticAlgorithmParams
{
    size_t const nbPenaltyManagement;
    size_t const repairProbability;

    bool const collectStatistics;
    bool const shouldIntensify;

    GeneticAlgorithmParams(size_t nbPenaltyManagement = 47,
                 size_t repairProbability = 79,
                 bool collectStatistics = false,
                 bool shouldIntensify = true)
        : nbPenaltyManagement(nbPenaltyManagement),
          repairProbability(repairProbability),
          collectStatistics(collectStatistics),
          shouldIntensify(shouldIntensify){};
};

#endif  // HGS_GENETICALGORITHMPARAMS_H
