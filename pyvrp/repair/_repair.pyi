from pyvrp import CostEvaluator, DynamicBitset, ProblemData, Solution

def greedy_repair(
    solution: Solution,
    unplanned: DynamicBitset,
    data: ProblemData,
    cost_evaluator: CostEvaluator,
) -> Solution: ...
