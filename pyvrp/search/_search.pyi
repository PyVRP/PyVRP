from typing import Iterator

from pyvrp._pyvrp import (
    CostEvaluator,
    DistanceSegment,
    DurationSegment,
    LoadSegment,
    ProblemData,
    RandomNumberGenerator,
    Solution,
)

class NodeOperator:
    def __init__(self, data: ProblemData) -> None: ...
    def evaluate(
        self, U: Node, V: Node, cost_evaluator: CostEvaluator
    ) -> int: ...
    def apply(self, U: Node, V: Node) -> None: ...

class RouteOperator:
    def __init__(self, data: ProblemData) -> None: ...
    def evaluate(
        self, U: Route, V: Route, cost_evaluator: CostEvaluator
    ) -> int: ...
    def apply(self, U: Route, V: Route) -> None: ...

class Exchange10(NodeOperator): ...
class Exchange11(NodeOperator): ...
class Exchange20(NodeOperator): ...
class Exchange21(NodeOperator): ...
class Exchange22(NodeOperator): ...
class Exchange30(NodeOperator): ...
class Exchange31(NodeOperator): ...
class Exchange32(NodeOperator): ...
class Exchange33(NodeOperator): ...
class SwapRoutes(RouteOperator): ...
class SwapStar(RouteOperator): ...
class SwapTails(NodeOperator): ...

class LocalSearch:
    def __init__(
        self,
        data: ProblemData,
        neighbours: list[list[int]],
    ) -> None: ...
    def add_node_operator(self, op: NodeOperator) -> None: ...
    def add_route_operator(self, op: RouteOperator) -> None: ...
    def set_neighbours(self, neighbours: list[list[int]]) -> None: ...
    def neighbours(self) -> list[list[int]]: ...
    def __call__(
        self,
        solution: Solution,
        cost_evaluator: CostEvaluator,
    ) -> Solution: ...
    def shuffle(self, rng: RandomNumberGenerator) -> None: ...
    def intensify(
        self,
        solution: Solution,
        cost_evaluator: CostEvaluator,
        overlap_tolerance: float = 0.05,
    ) -> Solution: ...
    def search(
        self, solution: Solution, cost_evaluator: CostEvaluator
    ) -> Solution: ...

class Route:
    def __init__(
        self, data: ProblemData, idx: int, vehicle_type: int
    ) -> None: ...
    @property
    def idx(self) -> int: ...
    @property
    def vehicle_type(self) -> int: ...
    def __delitem__(self, idx: int) -> None: ...
    def __getitem__(self, idx: int) -> Node: ...
    def __iter__(self) -> Iterator[Node]: ...
    def __len__(self) -> int: ...
    def is_feasible(self) -> bool: ...
    def has_excess_load(self) -> bool: ...
    def has_excess_distance(self) -> bool: ...
    def has_time_warp(self) -> bool: ...
    def capacity(self, dimension: int = 0) -> int: ...
    def start_depot(self) -> int: ...
    def end_depot(self) -> int: ...
    def fixed_vehicle_cost(self) -> int: ...
    def load(self, dimension: int = 0) -> int: ...
    def excess_load(self, dimension: int = 0) -> int: ...
    def excess_distance(self) -> int: ...
    def distance(self) -> int: ...
    def distance_cost(self) -> int: ...
    def unit_distance_cost(self) -> int: ...
    def duration(self) -> int: ...
    def duration_cost(self) -> int: ...
    def unit_duration_cost(self) -> int: ...
    def max_duration(self) -> int: ...
    def max_distance(self) -> int: ...
    def time_warp(self) -> int: ...
    def profile(self) -> int: ...
    def dist_at(self, idx: int, profile: int = 0) -> DistanceSegment: ...
    def dist_between(
        self, start: int, end: int, profile: int = 0
    ) -> DistanceSegment: ...
    def dist_before(self, end: int, profile: int = 0) -> DistanceSegment: ...
    def dist_after(self, start: int, profile: int = 0) -> DistanceSegment: ...
    def load_at(self, idx: int, dimension: int = 0) -> LoadSegment: ...
    def load_between(
        self, start: int, end: int, dimension: int = 0
    ) -> LoadSegment: ...
    def load_before(self, end: int, dimension: int = 0) -> LoadSegment: ...
    def load_after(self, start: int, dimension: int = 0) -> LoadSegment: ...
    def duration_at(self, idx: int, profile: int = 0) -> DurationSegment: ...
    def duration_between(
        self, start: int, end: int, profile: int = 0
    ) -> DurationSegment: ...
    def duration_before(
        self, end: int, profile: int = 0
    ) -> DurationSegment: ...
    def duration_after(
        self, start: int, profile: int = 0
    ) -> DurationSegment: ...
    def overlaps_with(self, other: Route, tolerance: float) -> bool: ...
    def centroid(self) -> tuple[float, float]: ...
    def append(self, node: Node) -> None: ...
    def clear(self) -> None: ...
    def insert(self, idx: int, node: Node) -> None: ...
    def update(self) -> None: ...

class Node:
    def __init__(self, loc: int) -> None: ...
    @property
    def client(self) -> int: ...
    @property
    def idx(self) -> int: ...
    @property
    def route(self) -> Route | None: ...
    def is_depot(self) -> bool: ...

def insert_cost(
    U: Node, V: Node, data: ProblemData, cost_evaluator: CostEvaluator
) -> int: ...
def inplace_cost(
    U: Node, V: Node, data: ProblemData, cost_evaluator: CostEvaluator
) -> int: ...
def remove_cost(
    U: Node, data: ProblemData, cost_evaluator: CostEvaluator
) -> int: ...
