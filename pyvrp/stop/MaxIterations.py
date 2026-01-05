class MaxIterations:
    """
    Criterion that stops after a maximum number of iterations.
    """

    def __init__(self, max_iterations: int):
        if max_iterations < 0:
            raise ValueError("max_iterations < 0 not understood.")

        self._max_iters = max_iterations
        self._curr_iter = 0

    def __call__(self, best_cost: float) -> bool:
        self._curr_iter += 1

        return self._curr_iter > self._max_iters
