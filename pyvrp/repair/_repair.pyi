from typing import List

from pyvrp._pyvrp import (
    CostEvaluator,
    DynamicBitset,
    ProblemData,
    Route,
    Solution,
)

def greedy_repair(
    routes: List[Route],
    to_insert: DynamicBitset,
    data: ProblemData,
    cost_evaluator: CostEvaluator,
) -> Solution: ...
