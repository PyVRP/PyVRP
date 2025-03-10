from collections import deque
from statistics import mean
from time import perf_counter


class MovingAverageThreshold:
    """
    The Moving Average Threshold (MAT) criterion of [1].

    Parameters
    ----------
    eta: float
        Used to determine the threshold value. Larger values of :math:`\\eta`
        result in more accepted candidate solutions. Must be in [0, 1].
    history_size: int
        History size. Must be positive.

    References
    ----------
    .. [1] Máximo, V.R. and M.C.V. Nascimento. 2021. A hybrid adaptive iterated
           local search with diversification control to the capacitated vehicle
           routing problem, *European Journal of Operational Research* 294 (3):
           1108 - 1119.
    """

    def __init__(
        self, eta: float, history_size: int, max_runtime: float = float("inf")
    ):
        if not (0 <= eta <= 1):
            raise ValueError("eta must be in [0, 1].")

        if history_size <= 0:
            raise ValueError("history_size must be positive.")

        self._eta = eta
        self._history_size = history_size
        self._max_runtime = max_runtime
        self._history: deque[float] = deque(maxlen=history_size)
        self._start_time = None
        self._num_improving = 0

    @property
    def eta(self) -> float:
        return self._eta

    @property
    def history_size(self) -> int:
        return self._history_size

    @property
    def max_runtime(self) -> float:
        return self._max_runtime

    @property
    def history(self) -> list[float]:
        return list(self._history)

    @property
    def threshold(self) -> float:
        if self._start_time is None:
            self._start_time = perf_counter()

        if not self._history:
            return float("inf")

        recent_best = min(self._history)
        recent_avg = mean(self._history)
        pct_time = (perf_counter() - self._start_time) / self.max_runtime
        factor = max(self._eta * (1 - pct_time), 0.01)

        return recent_best + factor * (recent_avg - recent_best)

    def __call__(self, best: float, current: float, candidate: float) -> bool:
        self._history.append(candidate)
        return candidate <= self.threshold
