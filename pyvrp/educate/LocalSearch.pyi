from typing import List, overload

from pyvrp.Individual import Individual
from pyvrp.PenaltyManager import PenaltyManager
from pyvrp.ProblemData import ProblemData
from pyvrp.XorShift128 import XorShift128

Neighbours = List[List[int]]

class LocalSearch:
    @overload
    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: XorShift128,
        params: LocalSearchParams,
    ) -> None: ...
    @overload
    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: XorShift128,
    ) -> None: ...
    def add_node_operator(self, op) -> None: ...
    def add_route_operator(self, op) -> None: ...
    def set_neighbours(self, Neighbours) -> None: ...
    def get_neighbours(self) -> Neighbours: ...
    def intensify(self, individual: Individual) -> None: ...
    def search(self, individual: Individual) -> None: ...

class LocalSearchParams:
    def __init__(
        self,
        weight_wait_time: int = ...,
        weight_time_warp: int = ...,
        nb_granular: int = ...,
        post_process_path_length: int = ...,
    ) -> None: ...
    @property
    def nb_granular(self) -> int: ...
    @property
    def post_process_path_length(self) -> int: ...
    @property
    def weight_time_warp(self) -> int: ...
    @property
    def weight_wait_time(self) -> int: ...
