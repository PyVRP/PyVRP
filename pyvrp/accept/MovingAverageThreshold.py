from time import perf_counter

import numpy as np


class MovingAverageThreshold:
    """
    The Moving Average Threshold (MAT) criterion of [1]_. This criterion
    accepts a candidate solution if it is better than a threshold value that is
    based on the moving average of the objective values of recently observed
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

    The algorithm incorporates a decreasing factor :math:`\\tilde{\\eta}` that
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
        :math:`\\tilde{\\eta} \\to 0`. Must be non-negative. Default is
        ``None``, meaning that :math:`\\tilde{\\eta}` stays constant.
    max_iterations
        Maximum number of iterations. As the search approaches this limit,
        :math:`\\tilde{\\eta} \\to 0`. Must be non-negative. Default is
        ``None``, meaning that :math:`\\tilde{\\eta}` stays constant.

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
        max_runtime: float | None = None,
        max_iterations: int | None = None,
    ):
        if eta < 0:
            raise ValueError("eta must be non-negative.")

        if gamma <= 0:
            raise ValueError("gamma must be positive.")

        if max_runtime is not None and max_runtime < 0:
            raise ValueError("max_runtime must be non-negative.")

        if max_iterations is not None and max_iterations < 0:
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
    def max_runtime(self) -> float | None:
        return self._max_runtime

    @property
    def max_iterations(self) -> float | None:
        return self._max_iterations

    @property
    def _runtime_budget(self) -> float:
        """
        Returns the remaining runtime budget as percentage of the maximum
        runtime.
        """
        if self.max_runtime is None:
            return 1

        if self.max_runtime == 0:
            return 0

        runtime = perf_counter() - self._start_time
        return max(1 - runtime / self.max_runtime, 0)

    @property
    def _iteration_budget(self) -> float:
        """
        Returns the remaining iteration budget as percentage of the maximum
        iterations.
        """
        if self.max_iterations is None:
            return 1

        if self.max_iterations == 0:
            return 0

        return 1 - self._iters / self.max_iterations

    def __call__(self, best: float, current: float, candidate: float) -> bool:
        idx = self._iters % self.gamma
        self._history[idx] = candidate

        history = self._history
        if self._iters < self.gamma:  # not yet enough solutions observed
            history = history[: self._iters + 1]

        recent_best = history.min()
        recent_avg = history.mean()
        factor = self.eta * min(self._runtime_budget, self._iteration_budget)

        self._iters += 1

        return candidate <= recent_best + factor * (recent_avg - recent_best)
