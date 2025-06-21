import time


class MaxRuntime:
    """
    Criterion that stops after a specified maximum runtime (in seconds).
    """

    def __init__(self, max_runtime: float):
        if max_runtime < 0:
            raise ValueError("max_runtime < 0 not understood.")

        self._max_runtime = max_runtime
        self._start_runtime: float | None = None

    @property
    def max_runtime(self) -> float:
        return self._max_runtime

    def fraction_remaining(self) -> float:
        if self._max_runtime == 0:
            return 0

        if self._start_runtime is None:
            return 1

        elapsed_time = time.perf_counter() - self._start_runtime
        remaining_time = self._max_runtime - elapsed_time
        return max(remaining_time / self._max_runtime, 0)

    def __call__(self, best_cost: float) -> bool:
        if self._start_runtime is None:
            self._start_runtime = time.perf_counter()

        return time.perf_counter() - self._start_runtime > self._max_runtime
