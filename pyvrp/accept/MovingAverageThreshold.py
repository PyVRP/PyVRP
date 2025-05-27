from time import perf_counter

import numpy as np


class MovingAverageThreshold:
    """
    The Moving Average Threshold (MAT) criterion of [1]. This criterion accepts
    a candidate solution if it is better than a threshold value that is based
    on the moving average of the objective values of recently observed
    candidate solutions. The threshold is computed as:

    .. math::

       f(s^*) + \\tilde{\\eta} \\times \\left(
          \\sum_{j = 1}^\\gamma \\frac{f(s^j)}{\\gamma} - f(s^*)
       \\right)

    where :math:`s^*` is the best solution observed in the last :math:`\\gamma`
    iterations, :math:`f(\\cdot)` indicates the objective function,
    :math:`\\gamma \\in \\mathbb{N}` is a parameter, and each :math:`s^j` is a
    recently observed solution. The recently observed solutions are stored in
    a ``history`` attributed of size at most :math:`\\gamma`.

    The algorithm incorporates a decreasing :math:`\\tilde{\\eta}` factor that
    converges to zero as the search approaches its maximum time limit or
    iterations. It is calculated as:

    .. math::
        \\tilde{\\eta} = \\eta \\times \\min\\left( 1 - \\frac{t}{T},
            1 - \\frac{i}{I} \\right)

    where :math:`\\eta \\ge 0` is a parameter, :math:`t` is the time elapsed
    since the start of the algorithm, :math:`T` is the imposed maximum runtime,
    :math:`i` is the number of iterations performed, and :math:`I` is the
    maximum number of iterations allowed. The most restrictive limit between
    the two is used to compute the factor.

    Parameters
    ----------
    eta
        Used to determine the threshold value. Larger values of :math:`\\eta`
        result in more accepted candidate solutions. Must be non-negative.
    gamma
        History size. Must be positive.
    max_runtime
        Maximum runtime in seconds. As the search approaches this time limit,
        :math:`\\eta` converges to zero. Must be non-negative. Default is
        ``float("inf")``, meaning that :math:`\\eta` stays constant.
    max_iterations
        Maximum number of iterations. As the search approaches this limit,
        :math:`\\eta` converges to zero. Must be non-negative. Default is
        ``float("inf")``, meaning that :math:`\\eta` stays constant.

    References
    ----------
    .. [1] Máximo, V.R. and M.C.V. Nascimento. 2021. A hybrid adaptive iterated
           local search with diversification control to the capacitated vehicle
           routing problem, *European Journal of Operational Research* 294 (3):
           1108 - 1119.
    """

    def __init__(
        self,
        eta: float,
        gamma: int,
        max_runtime: float = float("inf"),
        max_iterations: float = float("inf"),
    ):
        if eta < 0:
            raise ValueError("eta must be non-negative.")

        if gamma <= 0:
            raise ValueError("gamma must be positive.")

        if max_runtime < 0:
            raise ValueError("max_runtime must be non-negative.")

        if max_iterations < 0:
            raise ValueError("max_iterations must be non-negative.")

        self._eta = eta
        self._gamma = gamma
        self._max_runtime = max_runtime
        self._max_iterations = max_iterations

        self._history = np.zeros(gamma)
        self._start_time = perf_counter()
        self._iters = 0

    @property
    def eta(self) -> float:
        return self._eta

    @property
    def gamma(self) -> int:
        return self._gamma

    @property
    def max_runtime(self) -> float:
        return self._max_runtime

    @property
    def max_iterations(self) -> float:
        return self._max_iterations

    def __call__(self, best: float, current: float, candidate: float) -> bool:
        idx = self._iters % self.gamma
        self._history[idx] = candidate

        history = self._history
        if self._iters < self.gamma:  # not yet enough solutions observed
            history = history[: self._iters + 1]

        recent_best = history.min()
        recent_avg = history.mean()

        runtime = min(perf_counter() - self._start_time, self.max_runtime)
        pct_runtime = runtime / self.max_runtime if self.max_runtime else 1
        pct_iters = (
            self._iters / self.max_iterations if self.max_iterations else 1
        )
        factor = self.eta * min(1 - pct_runtime, 1 - pct_iters)

        self._iters += 1

        return candidate <= recent_best + factor * (recent_avg - recent_best)
