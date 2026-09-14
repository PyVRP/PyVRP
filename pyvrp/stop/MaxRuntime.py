import time


class MaxRuntime:
    """
    Criterion that stops after a specified maximum runtime (in seconds).

    .. note::
       This criterion limits the runtime of the
       :class:`~pyvrp.IteratedLocalSearch.IteratedLocalSearch` algorithm.
       PyVRP's :meth:`~pyvrp.solve.solve` also does some initial set-up before
       entering the algorithm. The set-up time is not counted towards the
       maximum algorithm runtime.
    """

    def __init__(self, max_runtime: float):
        if max_runtime < 0:
            raise ValueError("max_runtime < 0 not understood.")

        self._max_runtime = max_runtime
        self._start_runtime: float | None = None

    def __call__(self, best_cost: int) -> bool:
        if self._start_runtime is None:
            self._start_runtime = time.perf_counter()

        return time.perf_counter() - self._start_runtime > self._max_runtime
