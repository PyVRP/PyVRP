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
    ):
        self._dr = _DestroyRepair(data, rng, neighbours)

    def __call__(
        self,
        solution: Solution,
        cost_evaluator: CostEvaluator,
        num_destroy: int,
    ) -> Solution:
        return self._dr(solution, cost_evaluator, num_destroy)
