class MaxIterations:
    """
    Criterion that stops after a maximum number of iterations.
    """

    def __init__(self, max_iterations: int):
        if max_iterations < 0:
            raise ValueError("max_iterations < 0 not understood.")

        self._max_iters = max_iterations
        self._curr_iter = 0

    def fraction_remaining(self) -> float:
        if self._max_iters == 0:
            return 0

        remaining_iters = self._max_iters - self._curr_iter
        return max(remaining_iters / self._max_iters, 0.0)

    def __call__(self, best_cost: float) -> bool:
        self._curr_iter += 1

        return self._curr_iter > self._max_iters
