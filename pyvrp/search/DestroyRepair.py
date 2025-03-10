import time

from pyvrp._pyvrp import (
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Solution,
)
from pyvrp.search._search import DestroyRepair as _DestroyRepair


class DestroyRepair:
    def __init__(
        self,
        data: ProblemData,
        rng: RandomNumberGenerator,
        neighbours: list[list[int]],
        max_runtime: float,
    ):
        self._dr = _DestroyRepair(data, rng, neighbours)
        self._rng = rng

        self._start_time = time.perf_counter()
        self._max_runtime = max_runtime

    def __call__(
        self,
        solution: Solution,
        cost_evaluator: CostEvaluator,
    ) -> Solution:

        delta = time.perf_counter() - self._start_time
        elapsed = 1 - delta / self._max_runtime

        min_destroy = 10
        max_destroy = 25
        delta = max_destroy - min_destroy

        num_destroy = min_destroy + self._rng.randint(
            int(elapsed * delta) * 2 + 1
        )

        return self._dr(solution, cost_evaluator, num_destroy)
