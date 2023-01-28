#ifndef HGS_POPULATIONPARAMS_H
#define HGS_POPULATIONPARAMS_H

#include <iosfwd>

struct PopulationParams
{
    size_t const minPopSize;
    size_t const generationSize;
    size_t const nbElite;
    size_t const nbClose;

    double const lbDiversity;
    double const ubDiversity;

    PopulationParams(size_t minPopSize = 25,
                     size_t generationSize = 40,
                     size_t nbElite = 4,
                     size_t nbClose = 5,
                     double lbDiversity = 0.1,
                     double ubDiversity = 0.5)
        : minPopSize(minPopSize),
          generationSize(generationSize),
          nbElite(nbElite),
          nbClose(nbClose),
          lbDiversity(lbDiversity),
          ubDiversity(ubDiversity)
    {
        // TODO parameter validation
    }
};

#endif  // HGS_POPULATIONPARAMS_H
