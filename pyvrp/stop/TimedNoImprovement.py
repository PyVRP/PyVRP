from pyvrp.stop.MaxRuntime import MaxRuntime
from pyvrp.stop.NoImprovement import NoImprovement


class TimedNoImprovement:
    """
    Stopping criterion that stops after a given number of iterations without
    improvement, or after a fixed runtime (whichever happens first).
    """

    def __init__(self, max_iterations: int, max_runtime: float):
        self._no_improvement = NoImprovement(max_iterations)
        self._max_runtime = MaxRuntime(max_runtime)

    def __call__(self, best_cost: float) -> bool:
        return self._no_improvement(best_cost) or self._max_runtime(best_cost)
