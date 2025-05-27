from time import perf_counter

import numpy as np


class MovingAverageThreshold:
    R"""
    The Moving Average Threshold (MAT) criterion of [1]_. This criterion
    accepts a candidate solution if it is better than a threshold value that is
    based on the moving average of the objective values of recently observed
    candidate solutions. The threshold is computed as:

    .. math::

       f(s^*) + w \times \left(\sum_{j = 1}^N \frac{f(s^j)}{N} - f(s^*) \right)

    where :math:`s^*` is the best solution observed in the last :math:`N`
    iterations, :math:`f(\cdot)` is the objective function,
    :math:`N \in \mathbb{N}` is the history length parameter, and each
    :math:`s^j` is a recently observed solution.

    The dynamic weight :math:`w` converges to zero as the search approaches
    its maximum runtime or iterations. It is calculated as:

    .. math::
        w = w_0 \times \min\left(1 - \frac{t}{T}, 1 - \frac{i}{I} \right)

    where :math:`w_0 \ge 0` is the initial weight parameter, :math:`T \ge 0`
    and :math:`I \ge 0` are the maximum runtime and iterations parameters,
    and :math:`t` and :math:`i` are the elapsed runtime and number of
    iterations. The dynamic weight uses whichever limit (runtime or iterations)
    is most restrictive.

    Note: The parameters ``weight`` and ``history_length`` correspond to
    :math:`\eta` and :math:`\gamma` respectively in [1]_.

    Parameters
    ----------
    weight
        Initial weight parameter :math:`w_0` used to determine the threshold
        value. Larger values result in more accepted candidate solutions. Must
        be non-negative.
    history_length
        The number of recent candidate solutions to consider when computing
        the threshold value. Must be positive.
    max_runtime
        Maximum runtime in seconds. As the search approaches this time limit,
        :math:`w \to 0`. Must be non-negative. Default is ``None``, meaning
        that :math:`w` stays equal to :math:`w_0`.
    max_iterations
        Maximum number of iterations. As the search approaches this limit,
        :math:`w \to 0`. Must be non-negative. Default is ``None``, meaning
        that :math:`w` stays equal to :math:`w_0`.

    References
    ----------
    .. [1] Máximo, V.R. and M.C.V. Nascimento. 2021. A hybrid adaptive iterated
           local search with diversification control to the capacitated vehicle
           routing problem, *European Journal of Operational Research* 294 (3):
           1108 - 1119.
    """

    def __init__(
        self,
        weight: float,
        history_length: int,
        max_runtime: float | None = None,
        max_iterations: int | None = None,
    ):
        if weight < 0:
            raise ValueError("weight must be non-negative.")

        if history_length <= 0:
            raise ValueError("history_length must be positive.")

        if max_runtime is not None and max_runtime < 0:
            raise ValueError("max_runtime must be non-negative.")

        if max_iterations is not None and max_iterations < 0:
            raise ValueError("max_iterations must be non-negative.")

        self._weight = weight
        self._history_length = history_length
        self._max_runtime = max_runtime
        self._max_iterations = max_iterations

        self._history = np.zeros(history_length)
        self._start_time = perf_counter()
        self._iters = 0

    @property
    def weight(self) -> float:
        return self._weight

    @property
    def history_length(self) -> int:
        return self._history_length

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
        return 1 - runtime / self.max_runtime

    @property
    def _iteration_budget(self) -> float:
        """
        Returns the remaining iteration budget as percentage of the maximum
        number of iterations.
        """
        if self.max_iterations is None:
            return 1

        if self.max_iterations == 0:
            return 0

        return 1 - self._iters / self.max_iterations

    def __call__(self, best: float, current: float, candidate: float) -> bool:
        idx = self._iters % self.history_length
        self._history[idx] = candidate

        history = self._history
        if self._iters < self.history_length:  # not enough solutions observed
            history = history[: self._iters + 1]

        recent_best = history.min()
        recent_avg = history.mean()
        budget = min(self._runtime_budget, self._iteration_budget)
        weight = self.weight * budget

        self._iters += 1

        return candidate <= recent_best + weight * (recent_avg - recent_best)
