import time


class RecordToRecord:
    def __init__(
        self,
        start_threshold: float,
        end_threshold: float,
        max_runtime: float,
    ):
        self._start_threshold = start_threshold
        self._end_threshold = end_threshold
        self._max_runtime = max_runtime

        self._delta_threshold = start_threshold - end_threshold
        self._start_time = time.perf_counter()
        self._best = None

    @property
    def start_threshold(self) -> float:
        return self._start_threshold

    @property
    def end_threshold(self) -> float:
        return self._end_threshold

    @property
    def max_runtime(self) -> float:
        return self._max_runtime

    def __call__(self, best, curr, cand):
        if self._best is None:
            self._best = best

        now = time.perf_counter()
        pct_elapsed = (now - self._start_time) / self._max_runtime
        pct_elapsed = min(pct_elapsed, 1)
        threshold = (1 - pct_elapsed) * self._start_threshold * self._best

        return cand - best <= threshold
