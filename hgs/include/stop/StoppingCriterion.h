#ifndef STOPPINGCRITERION_H
#define STOPPINGCRITERION_H

/**
 * Base class for a stopping criterion.
 *
 * A stopping criterion has a callable member that returns a boolean that's
 * true the first time the algorithm should stop. Otherwise false.
 */
class StoppingCriterion
{
public:
    // TODO maybe pass in the best individual?
    virtual bool operator()(size_t const bestCost) = 0;

    virtual ~StoppingCriterion() = default;
};

#endif  // STOPPINGCRITERION_H
