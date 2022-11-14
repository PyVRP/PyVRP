#ifndef LOCALSEARCHOPERATOR_H
#define LOCALSEARCHOPERATOR_H

#include "Individual.h"
#include "Route.h"

template <typename Arg> class LocalSearchOperator
{
protected:
    Params const &d_params;

public:
    /**
     * Called once after loading in the individual to improve. This can be used
     * to e.g. update local operator state.
     */
    virtual void init(Individual const &indiv){};

    /**
     * Determines the cost delta of applying this operator to the arguments.
     * If the cost delta is negative, this is an improving move.
     * <br />
     * The contract is as follows: if the cost delta is negative, that is the
     * true cost delta of this move. As such, improving moves are fully
     * evaluated. The operator, however, is free to return early if it knows
     * the move will never be good: that is, when it determines the cost delta
     * cannot become negative at all. In that case, the returned (non-negative)
     * cost delta does not constitute a full evaluation.
     */
    virtual int evaluate(Arg *U, Arg *V) { return false; }

    /**
     * Applies this operator to the given arguments. For improvements, should
     * only be called if <code>evaluate()</code> returns a negative delta cost.
     */
    virtual void apply(Arg *U, Arg *V){};

    /**
     * Called when a route has been changed. Can be used to update caches, but
     * the implementation should be fast: this is called every time something
     * changes!
     */
    virtual void update(Route *U){};

    explicit LocalSearchOperator(Params const &params) : d_params(params) {}

    virtual ~LocalSearchOperator() = default;
};

#endif  // LOCALSEARCHOPERATOR_H
