from typing import overload

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

    @overload
    def __init__(self, capacity_penalty: int, tw_penalty: int) -> None: ...
    @overload
    def __init__(self) -> None: ...
    def load_penalty(self, load: int, vehicle_capacity: int) -> int: ...
    def tw_penalty(self, time_warp: int) -> int: ...
