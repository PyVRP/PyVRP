from collections import deque
from statistics import mean


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

    def __init__(self, eta: float, history_size: int):
        if not (0 <= eta <= 1):
            raise ValueError("eta must be in [0, 1].")

        if history_size <= 0:
            raise ValueError("history_size must be positive.")

        self._eta = eta
        self._history_size = history_size
        self._history: deque[float] = deque(maxlen=history_size)

    @property
    def eta(self) -> float:
        return self._eta

    @property
    def history_size(self) -> int:
        return self._history_size

    @property
    def history(self) -> list[float]:
        return list(self._history)

    def __call__(self, best: float, current: float, candidate: float) -> bool:
        self._history.append(candidate)
        recent_best = min(self._history)
        recent_avg = mean(self._history)

        threshold = recent_best + self._eta * (recent_avg - recent_best)
        return candidate <= threshold
