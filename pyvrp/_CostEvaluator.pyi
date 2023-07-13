from typing import overload

from pyvrp._Solution import Solution

class CostEvaluator:
    """
    Creates a CostEvaluator instance.

    This class contains time warp and load penalties, and can compute penalties
    for a given time warp and load.

    Parameters
    ----------
    weight_capacity_penalty
        The penalty for each unit of excess weight load over the vehicle weight capacity.
    volume_capacity_penalty
        The penalty for each unit of excess volume load over the vehicle volume capacity.
    salvage_capacity_penalty
        The penalty for each nonterminal salvage pickup.
    salvage_sequence_penalty
        The penalty for each nonterminal salvage pickup.
    tw_penalty
        The penalty for each unit of time warp.
    """

    def __init__(
        self, 
        weight_capacity_penalty: int = 0, 
        volume_capacity_penalty: int = 0, 
        salvage_penalty: int = 0, 
//        salvage_sequence_penalty: int = 0, 
        tw_penalty: int = 0
    ) -> None: ...
    def weight_penalty(self, weight: int, weight_capacity: int) -> int: ...
    def volume_penalty(self, volume: int, volume_capacity: int) -> int: ...
    def salvage_penalty(self, salvage: int, salvage_capacity: int) -> int: ...
//    def salvage_sequence_penalty(self, salvage: int, salvage_capacity: int) -> int: ...
    def tw_penalty(self, time_warp: int) -> int: ...
    def penalised_cost(self, solution: Solution) -> int: ...
    def cost(self, solution: Solution) -> int:
        """
        Evaluates and returns the cost/objective of the given solution.
        Hand-waving some details, let :math:`x_{ij} \\in \\{ 0, 1 \\}` indicate
        if edge :math:`(i, j)` is used in the solution encoded by the given
        solution, and :math:`y_i \\in \\{ 0, 1 \\}` indicate if client
        :math:`i` is visited. The objective is then given by

        .. math::

           \\sum_{(i, j)} d_{ij} x_{ij} + \\sum_{i} p_i (1 - y_i),

        where the first part lists the distance costs, and the second part the
        prizes of the unvisited clients.
        """
