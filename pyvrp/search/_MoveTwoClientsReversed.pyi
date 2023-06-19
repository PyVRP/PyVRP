from pyvrp import ProblemData

class NodeOperator:
    def __init__(self, *args, **kwargs) -> None: ...

class MoveTwoClientsReversed(NodeOperator):
    def __init__(self, data: ProblemData) -> None: ...
