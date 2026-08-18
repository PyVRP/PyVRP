from pyvrp.constants import INT_MAX


class FirstFeasible:
    """
    Terminates the search after a feasible solution has been observed.
    """

    def __call__(self, best_cost: int) -> bool:
        # This function is called with the output of CostEvaluator.cost on the
        # best solution, which is INT_MAX when the best solution is infeasible.
        # Thus, when the cost is below INT_MAX, we have at least one feasible
        # solution and we can terminate.
        return best_cost < INT_MAX
