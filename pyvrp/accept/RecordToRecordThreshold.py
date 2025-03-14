from math import exp
from time import perf_counter


class RecordToRecordThreshold:
    def __init__(
        self, start_pct: float, end_pct: int, max_runtime: float = float("inf")
    ):
        self._start_pct = start_pct
        self._end_pct = end_pct
        self._max_runtime = max_runtime
        self._start_time = None
        self.best = float("inf")

    @property
    def start_pct(self) -> float:
        return self._start_pct

    @property
    def end_pct(self) -> int:
        return self._end_pct

    @property
    def max_runtime(self) -> float:
        return self._max_runtime

    @property
    def threshold(self) -> float:
        if self._start_time is None:
            self._start_time = perf_counter()

        delta_pct = self._start_pct - self._end_pct
        pct_time = (perf_counter() - self._start_time) / self.max_runtime
        delta = delta_pct * exp(-5 * pct_time)

        return self.best + self._end_pct + delta

    def __call__(self, best, current, candidate) -> bool:
        if best < self.best:
            self.best = best

        return candidate <= self.threshold
