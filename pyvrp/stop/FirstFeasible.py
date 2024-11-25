import sys


class FirstFeasible:
    """
    Terminates the search after a feasible solution has been observed.
    """

    def __call__(self, best_cost: float) -> bool:
        # This function is called with the output of CostEvaluator.cost on the
        # best solution, which is FLOAT_MAX when the solution is infeasible.
        # Thus, when the cost is below FLOAT_MAX, we have at least one feasible
        # solution and we can terminate.
        return best_cost < sys.float_info.max
