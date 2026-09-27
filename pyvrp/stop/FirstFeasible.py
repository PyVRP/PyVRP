import numpy as np

_FLOAT_MAX = np.finfo(np.float64).max


class FirstFeasible:
    """
    Terminates the search after a feasible solution has been observed.
    """

    def __call__(self, best_cost: float) -> bool:
        # This function is called with the output of CostEvaluator.cost on the
        # best solution, which is FLOAT_MAX for infeasible solutions. A cost
        # below FLOAT_MAX thus indicates a feasible solution.
        return best_cost < _FLOAT_MAX
