import numpy as np

from pyvrp.constants import TOL

_COST_MAX = np.finfo(np.float64).max


class FirstFeasible:
    """
    Terminates the search after a feasible solution has been observed.
    """

    def __call__(self, best_cost: float) -> bool:
        # This function is called with the output of CostEvaluator.cost on the
        # best solution. An infeasible solution has cost COST_MAX, so a lower
        # value means we have a feasible solution and can terminate.
        return best_cost < _COST_MAX - TOL
