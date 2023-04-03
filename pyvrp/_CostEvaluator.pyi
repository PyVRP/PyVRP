from typing import overload

from pyvrp._Individual import Individual

class CostEvaluator:
    """
    Creates a CostEvaluator instance.

    This class contains time warp and load penalties, and can compute penalties
    for a given time warp and load.

    Parameters
    ----------
    capacity_penalty
        The penalty for each unit of excess load over the vehicle capacity.
    tw_penalty
        The penalty for each unit of time warp.
    """

    def __init__(
        self, capacity_penalty: float = 0, tw_penalty: float = 0
    ) -> None: ...
    def load_penalty(self, load: int, vehicle_capacity: int) -> float: ...
    def tw_penalty(self, time_warp: float) -> float: ...
    def penalised_cost(self, individual: Individual) -> float: ...
    def cost(self, individual: Individual) -> float: ...
