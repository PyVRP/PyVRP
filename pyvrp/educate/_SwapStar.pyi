from pyvrp import PenaltyManager, ProblemData

class RouteOperator:
    def __init__(self, *args, **kwargs) -> None: ...

class SwapStar(RouteOperator):
    def __init__(
        self, data: ProblemData, penalty_manager: PenaltyManager
    ) -> None: ...
