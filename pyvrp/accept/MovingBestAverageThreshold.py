from time import perf_counter

import numpy as np


class MovingBestAverageThreshold:
    R"""
    The Moving Best Average Threshold criterion of [1]_. This criterion accepts
    a candidate solution if it is better than a threshold value. This threshold
    is based on the objective values of recently observed candidate solutions.
    Specifically, it is a convex combination of the recent best and average
    values, computed as:

    .. math::

       (1 - w) \times f(s^*) + w \times \sum_{j = 1}^N \frac{f(s^j)}{N}

    where :math:`s^*` is the best solution observed in the last :math:`N`
    iterations, :math:`f(\cdot)` is the objective function,
    :math:`N \in \mathbb{N}` is the history length parameter, and each
    :math:`s^j` is a recently observed solution.

    The weight :math:`w` is initially set at :math:`w_0 \in [0, 1]` and it
    converges to zero as the search approaches its maximum runtime or number
    of iterations. In each iteration, the weight is calculated as:

    .. math::
       w = w_0 \times \min\left(1 - \frac{t}{T}, 1 - \frac{i}{I} \right)

    where :math:`T \ge 0` and :math:`I \ge 0` are the maximum runtime and
    iterations parameters, and :math:`t` and :math:`i` are the elapsed runtime
    and number of iterations. The weight uses whichever limit (runtime or
    iterations) is most restrictive.

    .. note::
       The parameters :math:`w_0` and :math:`N` correspond to :math:`\eta`
       and :math:`\gamma` respectively in [1]_.

    Parameters
    ----------
    initial_weight
        Initial weight parameter :math:`w_0` used to determine the threshold
        value. Larger values result in more accepted candidate solutions. Must
        be in [0, 1].
    history_length
        The number of recent candidate solutions :math:`N` to consider when
        computing the threshold value. Must be positive.
    max_runtime
        Maximum runtime in seconds. As the search approaches this time limit,
        :math:`w \to 0`. Must be non-negative. Default is ``None``, meaning
        that the runtime limit is ignored when calculating the weight.
    max_iterations
        Maximum number of iterations. As the search approaches this limit,
        :math:`w \to 0`. Default is ``None``, meaning that the iterations limit
        is ignored when calculating the weight.

    References
    ----------
    .. [1] Máximo, V.R. and M.C.V. Nascimento. 2021. A hybrid adaptive iterated
           local search with diversification control to the capacitated vehicle
           routing problem, *European Journal of Operational Research* 294 (3):
           1108 - 1119. https://doi.org/10.1016/j.ejor.2021.02.024.
    """

    def __init__(
        self,
        initial_weight: float,
        history_length: int,
        max_runtime: float | None = None,
        max_iterations: int | None = None,
    ):
        if not (0 <= initial_weight <= 1):
            raise ValueError("initial_weight must be in [0, 1].")

        if history_length <= 0:
            raise ValueError("history_length must be positive.")

        if max_runtime is not None and max_runtime < 0:
            raise ValueError("max_runtime must be non-negative.")

        if max_iterations is not None and max_iterations < 0:
            raise ValueError("max_iterations must be non-negative.")

        self._initial_weight = initial_weight
        self._history_length = history_length
        self._max_runtime = max_runtime
        self._max_iterations = max_iterations

        self._history = np.zeros(history_length)
        self._start_time = perf_counter()
        self._iters = 0

    @property
    def initial_weight(self) -> float:
        return self._initial_weight

    @property
    def history_length(self) -> int:
        return self._history_length

    @property
    def max_runtime(self) -> float | None:
        return self._max_runtime

    @property
    def max_iterations(self) -> int | None:
        return self._max_iterations

    def _runtime_budget(self) -> float:
        """
        Returns the remaining runtime budget as percentage of the maximum
        runtime.
        """
        if self._max_runtime is None:
            return 1

        runtime = perf_counter() - self._start_time

        if self._max_runtime == 0 or runtime > self._max_runtime:
            return 0

        return 1 - runtime / self._max_runtime

    def _iteration_budget(self) -> float:
        """
        Returns the remaining iteration budget as percentage of the maximum
        number of iterations.
        """
        if self._max_iterations is None:
            return 1

        if self._max_iterations == 0 or self._iters > self._max_iterations:
            return 0

        return 1 - self._iters / self._max_iterations

    def __call__(self, best: float, current: float, candidate: float) -> bool:
        idx = self._iters % self._history_length
        self._history[idx] = candidate

        history = self._history
        if self._iters < self._history_length:  # not enough solutions observed
            history = history[: self._iters + 1]

        recent_best = history.min()
        recent_avg = history.mean()
        budget = min(self._runtime_budget(), self._iteration_budget())
        weight = self._initial_weight * budget

        self._iters += 1

        return candidate <= (1 - weight) * recent_best + weight * recent_avg
