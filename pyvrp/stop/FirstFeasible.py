import numpy as np

_INT_MAX = np.iinfo(np.int64).max


class FirstFeasible:
    """
    Terminates the search after a feasible solution has been observed.
    """

    def __call__(self, best_cost: float) -> bool:
        # This function is called with the output of CostEvaluator.cost on the
        # best solution, which is INT_MAX when the best solution is infeasible.
        # Thus, when the cost is below INT_MAX, we have at least one feasible
        # solution and we can terminate.
        return best_cost < _INT_MAX
