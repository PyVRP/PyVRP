class FirstFeasible:
    """
    Terminates the search after a feasible solution has been observed.
    """

    def __call__(self, best_cost: float) -> bool:
        # This function is called with the output of CostEvaluator.cost on the
        # best solution, which is infinity when the solution is infeasible.
        return best_cost < float("inf")
