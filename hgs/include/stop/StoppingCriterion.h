#ifndef STOPPINGCRITERION_H
#define STOPPINGCRITERION_H

class StoppingCriterion
{
public:
    // TODO maybe pass in the best individual?
    virtual bool operator()(size_t const bestCost) = 0;

    virtual ~StoppingCriterion() = default;
};

#endif  // STOPPINGCRITERION_H
