#ifndef STOPPINGCRITERION_H
#define STOPPINGCRITERION_H

class StoppingCriterion
{
public:
    virtual bool operator()() = 0;

    virtual ~StoppingCriterion() = default;
};

#endif  // STOPPINGCRITERION_H
