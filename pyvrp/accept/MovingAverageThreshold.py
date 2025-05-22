import numpy as np


class MovingAverageThreshold:
    """
    The Moving Average Threshold (MAT) criterion of [1]. This criterion accepts
    a candidate solution if it is better than a threshold value that is based
    on the moving average of the objective values of recently observed
    candidate solutions. The threshold is computed as:

    .. math::

       f(s^*) + \\eta \\left(
          \\sum_{i = 1}^\\gamma \\frac{f(s^i)}{\\gamma} - f(s^*)
       \\right)

    where :math:`s^*` is the best solution observed in the last :math:`\\gamma`
    iterations, :math:`f(\\cdot)` indicates the objective function,
    :math:`\\eta \\in [0, 1]` and :math:`\\gamma \\in \\mathbb{N}` are
    parameters, and each :math:`s^i` is a recently observed solution. The
    recently observed solutions are stored in a ``history`` attributed of size
    at most :math:`\\gamma`.

    Parameters
    ----------
    eta: float
        Used to determine the threshold value. Larger values of :math:`\\eta`
        result in more accepted candidate solutions.
    gamma: int
        History size. Must be positive.

    References
    ----------
    .. [1] Máximo, V.R. and M.C.V. Nascimento. 2021. A hybrid adaptive iterated
           local search with diversification control to the capacitated vehicle
           routing problem, *European Journal of Operational Research* 294 (3):
           1108 - 1119.
    """

    def __init__(self, eta: float, gamma: int):
        if eta < 0:
            raise ValueError("eta must be non-negative.")

        if gamma <= 0:
            raise ValueError("gamma must be positive.")

        self._eta = eta
        self._gamma = gamma

        self._history = np.zeros(gamma)
        self._idx = 0

    @property
    def eta(self) -> float:
        return self._eta

    @property
    def gamma(self) -> int:
        return self._gamma

    def __call__(self, best: float, current: float, candidate: float) -> bool:
        idx = self._idx % self.gamma
        self._history[idx] = candidate
        self._idx += 1

        history = self._history
        if self._idx < self.gamma:  # not enough solutions observed
            history = history[: self._idx]

        recent_best = history.min()
        recent_avg = history.mean()

        return candidate <= recent_best + self.eta * (recent_avg - recent_best)
