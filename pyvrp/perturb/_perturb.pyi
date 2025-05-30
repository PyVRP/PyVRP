from pyvrp._pyvrp import (
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Solution,
)
from pyvrp.search._search import Node, Route

class DestroyOperator:
    def __call__(
        self,
        nodes: list[Node],
        routes: list[Route],
        cost_evaluator: CostEvaluator,
        neighbours: list[list[int]],
        rng: RandomNumberGenerator,
    ) -> None: ...

class RepairOperator:
    def __call__(
        self,
        nodes: list[Node],
        routes: list[Route],
        cost_evaluator: CostEvaluator,
        neighbours: list[list[int]],
        rng: RandomNumberGenerator,
    ) -> None: ...

class DestroyRepair:
    def __init__(self, data: ProblemData) -> None: ...
    def add_destroy_operator(self, op: DestroyOperator) -> None: ...
    def add_repair_operator(self, op: RepairOperator) -> None: ...
    def __call__(
        self,
        solution: Solution,
        cost_evaluator: CostEvaluator,
        neighbours: list[list[int]],
        rng: RandomNumberGenerator,
    ) -> Solution: ...

class NeighbourRemoval(DestroyOperator):
    def __init__(
        self,
        data: ProblemData,
        num_removals: int,
    ) -> None: ...
    def __call__(
        self,
        nodes: list[Node],
        routes: list[Route],
        cost_evaluator: CostEvaluator,
        neighbours: list[list[int]],
        rng: RandomNumberGenerator,
    ) -> None: ...

class GreedyRepair(RepairOperator):
    def __init__(
        self,
        data: ProblemData,
        skip_optional_probability: int = 100,
    ) -> None: ...
    def __call__(
        self,
        nodes: list[Node],
        routes: list[Route],
        cost_evaluator: CostEvaluator,
        neighbours: list[list[int]],
        rng: RandomNumberGenerator,
    ) -> None: ...
