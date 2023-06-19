from pyvrp import ProblemData

class RouteOperator:
    def __init__(self, *args, **kwargs) -> None: ...

class RelocateStar(RouteOperator):
    def __init__(self, data: ProblemData) -> None: ...
