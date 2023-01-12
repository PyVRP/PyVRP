#ifndef HGS_LOCALSEARCHPARAMS_H
#define HGS_LOCALSEARCHPARAMS_H

#include <iosfwd>

struct LocalSearchParams
{
    size_t const weightWaitTime;
    size_t const weightTimeWarp;
    size_t const nbGranular;
    size_t const postProcessPathLength;

    LocalSearchParams(size_t weightWaitTime = 18,
                      size_t weightTimeWarp = 20,
                      size_t nbGranular = 34,
                      size_t postProcessPathLength = 7)
        : weightWaitTime(weightWaitTime),
          weightTimeWarp(weightTimeWarp),
          nbGranular(nbGranular),
          postProcessPathLength(postProcessPathLength){};
};

#endif  // HGS_LOCALSEARCHPARAMS_H
