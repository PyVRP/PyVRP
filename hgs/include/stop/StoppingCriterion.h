#ifndef STOPPINGCRITERION_H
#define STOPPINGCRITERION_H

#include "Individual.h"

class StoppingCriterion
{
public:
    virtual bool operator()(Individual const &best) = 0;

    virtual ~StoppingCriterion() = default;
};

#endif  // STOPPINGCRITERION_H
