from pyvrp import PenaltyManager, ProblemData

class NodeOperator:
    def __init__(self, *args, **kwargs) -> None: ...

class TwoOpt(NodeOperator):
    def __init__(
        self, data: ProblemData, penalty_manager: PenaltyManager
    ) -> None: ...
