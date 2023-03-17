#ifndef LOCALSEARCHOPERATOR_H
#define LOCALSEARCHOPERATOR_H

#include "CostEvaluator.h"
#include "Individual.h"
#include "Node.h"
#include "ProblemData.h"
#include "Route.h"

template <typename Arg> class LocalSearchOperatorBase
{
    // Can only be specialised into either a Node or Route operator; there
    // are no other types that are expected to work.
    static_assert(std::is_same<Arg, Node>::value
                  || std::is_same<Arg, Route>::value);

protected:
    ProblemData const &data;

public:
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
    virtual int evaluate(Arg *U, Arg *V, CostEvaluator const &costEvaluator)
    {
        return 0;
    }

    /**
     * Applies this operator to the given arguments. For improvements, should
     * only be called if <code>evaluate()</code> returns a negative delta cost.
     */
    virtual void apply(Arg *U, Arg *V) const {};

    LocalSearchOperatorBase(ProblemData const &data) : data(data){};
    virtual ~LocalSearchOperatorBase() = default;
};

template <typename Arg>
class LocalSearchOperator : public LocalSearchOperatorBase<Arg>
{
};

template <>  // specialisation for node operators
class LocalSearchOperator<Node> : public LocalSearchOperatorBase<Node>
{
    using LocalSearchOperatorBase::LocalSearchOperatorBase;
};

template <>  // specialisation for route operators
class LocalSearchOperator<Route> : public LocalSearchOperatorBase<Route>
{
    using LocalSearchOperatorBase::LocalSearchOperatorBase;

public:
    /**
     * Called once after loading in the individual to improve. This can be used
     * to e.g. update local operator state.
     */
    virtual void init(Individual const &indiv){};

    /**
     * Called when a route has been changed. Can be used to update caches, but
     * the implementation should be fast: this is called every time something
     * changes!
     */
    virtual void update(Route *U){};
};

#endif  // LOCALSEARCHOPERATOR_H
