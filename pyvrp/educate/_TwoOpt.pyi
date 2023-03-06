from pyvrp import ProblemData

class NodeOperator:
    def __init__(self, *args, **kwargs) -> None: ...

class TwoOpt(NodeOperator):
    def __init__(self, data: ProblemData) -> None: ...
