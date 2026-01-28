#ifndef PYVRP_SEARCH_LOCALSEARCHOPERATOR_H
#define PYVRP_SEARCH_LOCALSEARCHOPERATOR_H

#include "../Solution.h"  // pyvrp::Solution
#include "CostEvaluator.h"
#include "Measure.h"
#include "ProblemData.h"
#include "Route.h"

namespace pyvrp::search
{
/**
 * Simple data structure that tracks statistics about the number of times
 * an operator was evaluated and applied.
 *
 * Attributes
 * ----------
 * num_evaluations
 *     Number of evaluated moves.
 * num_applications
 *     Number of applied, improving moves.
 */
struct OperatorStatistics
{
    size_t numEvaluations = 0;
    size_t numApplications = 0;
};

template <std::same_as<Route::Node *>... Args> class LocalSearchOperator
{
    static_assert(sizeof...(Args) > 0);  // need at least one argument

protected:
    ProblemData const &data;
    mutable OperatorStatistics stats_;

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
    virtual Cost evaluate(Args... args, CostEvaluator const &costEvaluator) = 0;

    /**
     * Applies this operator to the given arguments. For improvements, should
     * only be called if <code>evaluate()</code> returns a negative delta cost.
     */
    // TODO remove arguments - always applies to most recently evaluated pair.
    virtual void apply(Args... args) const = 0;

    /**
     * Called once after loading the solution to improve. This can be used to
     * e.g. update local operator state.
     */
    virtual void init([[maybe_unused]] pyvrp::Solution const &solution)
    {
        stats_ = {};  // reset call statistics
    };

    /**
     * Returns evaluation and application statistics collected since the last
     * solution initialisation.
     */
    OperatorStatistics const &statistics() const { return stats_; }

    LocalSearchOperator(ProblemData const &data) : data(data){};
    virtual ~LocalSearchOperator() = default;
};

using UnaryOperator = LocalSearchOperator<Route::Node *>;
using BinaryOperator = LocalSearchOperator<Route::Node *, Route::Node *>;

/**
 * Helper template function that may be specialised to determine if an operator
 * can find improving moves for the given data instance.
 */
template <typename Op> bool supports([[maybe_unused]] ProblemData const &data)
{
    return true;
}
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_LOCALSEARCHOPERATOR_H
