import time
from typing import Optional


class MaxRuntime:
    """
    Criterion that stops after a specified maximum runtime (in seconds).
    """

    def __init__(self, max_runtime: float):
        if max_runtime < 0:
            raise ValueError("max_runtime < 0 not understood.")

        self._max_runtime = max_runtime
        self._start_runtime: Optional[float] = None

    def __call__(self, best_cost: float) -> bool:
        if self._start_runtime is None:
            self._start_runtime = time.perf_counter()

        return time.perf_counter() - self._start_runtime > self._max_runtime
