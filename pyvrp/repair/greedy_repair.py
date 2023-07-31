from typing import List

from pyvrp import CostEvaluator, DynamicBitset, ProblemData, Route, Solution

from ._repair import greedy_repair as cpp_greedy_repair


def greedy_repair(
    routes: List[Route],
    to_insert: DynamicBitset,
    data: ProblemData,
    cost_evaluator: CostEvaluator,
) -> Solution:
    return cpp_greedy_repair(routes, to_insert, data, cost_evaluator)
