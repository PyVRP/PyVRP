from typing import List

from pyvrp._CostEvaluator import CostEvaluator
from pyvrp._ProblemData import ProblemData
from pyvrp._Solution import Solution
from pyvrp._XorShift128 import XorShift128

Neighbours = List[List[int]]

class LocalSearch:
    def __init__(
        self,
        data: ProblemData,
        neighbours: Neighbours,
    ) -> None: ...
    def add_node_operator(self, op) -> None: ...
    def add_route_operator(self, op) -> None: ...
    def set_neighbours(self, neighbours: Neighbours) -> None: ...
    def get_neighbours(self) -> Neighbours: ...
    def shuffle(self, rng: XorShift128) -> None: ...
    def intensify(
        self,
        solution: Solution,
        cost_evaluator: CostEvaluator,
        overlap_tolerance_degrees: int = 0,
    ) -> Solution: ...
    def search(
        self, solution: Solution, cost_evaluator: CostEvaluator
    ) -> Solution: ...
