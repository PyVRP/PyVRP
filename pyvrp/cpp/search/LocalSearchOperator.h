#ifndef PYVRP_SEARCH_LOCALSEARCHOPERATOR_H
#define PYVRP_SEARCH_LOCALSEARCHOPERATOR_H

#include "../Solution.h"  // pyvrp::Solution
#include "CostEvaluator.h"
#include "Measure.h"
#include "ProblemData.h"
#include "Route.h"

#include <utility>

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
protected:
    ProblemData const &data;
    mutable OperatorStatistics stats_;

public:
    /**
     * Determines the cost delta of applying this move to the arguments. If the
     * cost delta is negative, this is an improving move. The second, boolean
     * return value indicates whether the operator believes the move should be
     * applied (i.e., improves the solution).
     *
     * Moves that the operator believes should be applied must be fully
     * evaluated. The operator, however, is free to return early if it knows
     * the move will never be good: that is, when it determines the cost delta
     * cannot become negative at all. In that case, the returned (non-negative)
     * cost delta may not be a complete evaluation.
     */
    virtual std::pair<Cost, bool> evaluate(Args... args,
                                           CostEvaluator const &costEvaluator)
        = 0;

    /**
     * Applies this move to the given arguments. Should only be called
     * when ``evaluate()`` suggests applying the move.
     */
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
