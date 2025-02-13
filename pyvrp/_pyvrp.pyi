from typing import Callable, Iterator, overload

import numpy as np

class CostEvaluator:
    def __init__(
        self,
        load_penalties: list[float],
        tw_penalty: float,
        dist_penalty: float,
    ) -> None: ...
    def load_penalty(
        self, load: int, capacity: int, dimension: int
    ) -> int: ...
    def tw_penalty(self, time_warp: int) -> int: ...
    def dist_penalty(self, distance: int, max_distance: int) -> int: ...
    def penalised_cost(self, solution: Solution) -> int: ...
    def cost(self, solution: Solution) -> int: ...

class DynamicBitset:
    def __init__(self, num_bits: int) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __getitem__(self, idx: int) -> bool: ...
    def __setitem__(self, idx: int, value: bool) -> None: ...
    def all(self) -> bool: ...
    def any(self) -> bool: ...
    def none(self) -> bool: ...
    def count(self) -> int: ...
    def __len__(self) -> int: ...
    def __or__(self, other: DynamicBitset) -> DynamicBitset: ...
    def __and__(self, other: DynamicBitset) -> DynamicBitset: ...
    def __xor__(self, other: DynamicBitset) -> DynamicBitset: ...
    def __invert__(self) -> DynamicBitset: ...
    def reset(self) -> DynamicBitset: ...

class Client:
    x: int
    y: int
    delivery: list[int]
    pickup: list[int]
    service_duration: int
    tw_early: int
    tw_late: int
    release_time: int
    prize: int
    required: bool
    group: int | None
    name: str
    def __init__(
        self,
        x: int,
        y: int,
        delivery: list[int] = [],
        pickup: list[int] = [],
        service_duration: int = 0,
        tw_early: int = 0,
        tw_late: int = ...,
        release_time: int = 0,
        prize: int = 0,
        required: bool = True,
        group: int | None = None,
        *,
        name: str = "",
    ) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __getstate__(self) -> tuple: ...
    def __setstate__(self, state: tuple, /) -> None: ...

class ClientGroup:
    required: bool
    mutually_exclusive: bool
    def __init__(
        self,
        clients: list[int] = [],
        required: bool = True,
    ) -> None: ...
    @property
    def clients(self) -> list[int]: ...
    def __len__(self) -> int: ...
    def __iter__(self) -> Iterator[int]: ...
    def add_client(self, client: int) -> None: ...
    def clear(self) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __getstate__(self) -> tuple: ...
    def __setstate__(self, state: tuple, /) -> None: ...

class Depot:
    x: int
    y: int
    name: str
    def __init__(
        self,
        x: int,
        y: int,
        *,
        name: str = "",
    ) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __getstate__(self) -> tuple: ...
    def __setstate__(self, state: tuple, /) -> None: ...

class Reload:
    depot: int
    tw_early: int
    tw_late: int
    load_duration: int
    def __init__(
        self,
        depot: int = 0,
        tw_early: int = 0,
        tw_late: int = ...,
        load_duration: int = 0,
    ) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def __getstate__(self) -> tuple: ...
    def __setstate__(self, state: tuple, /) -> None: ...

class VehicleType:
    num_available: int
    start_depot: int
    end_depot: int
    capacity: list[int]
    tw_early: int
    tw_late: int
    max_duration: int
    max_distance: int
    fixed_cost: int
    unit_distance_cost: int
    unit_duration_cost: int
    profile: int
    start_late: int
    initial_load: list[int]
    reloads: list[Reload]
    name: str
    def __init__(
        self,
        num_available: int = 1,
        capacity: list[int] = [],
        start_depot: int = 0,
        end_depot: int = 0,
        fixed_cost: int = 0,
        tw_early: int = 0,
        tw_late: int = ...,
        max_duration: int = ...,
        max_distance: int = ...,
        unit_distance_cost: int = 1,
        unit_duration_cost: int = 0,
        profile: int = 0,
        start_late: int | None = None,
        initial_load: list[int] = [],
        reloads: list[Reload] = [],
        *,
        name: str = "",
    ) -> None: ...
    def replace(
        self,
        num_available: int | None = None,
        capacity: list[int] | None = None,
        start_depot: int | None = None,
        end_depot: int | None = None,
        fixed_cost: int | None = None,
        tw_early: int | None = None,
        tw_late: int | None = None,
        max_duration: int | None = None,
        max_distance: int | None = None,
        unit_distance_cost: int | None = None,
        unit_duration_cost: int | None = None,
        profile: int | None = None,
        start_late: int | None = None,
        initial_load: list[int] | None = None,
        reloads: list[Reload] | None = None,
        *,
        name: str | None = None,
    ) -> VehicleType: ...
    def __eq__(self, other: object) -> bool: ...
    def __getstate__(self) -> tuple: ...
    def __setstate__(self, state: tuple, /) -> None: ...

class ProblemData:
    def __init__(
        self,
        clients: list[Client],
        depots: list[Depot],
        vehicle_types: list[VehicleType],
        distance_matrices: list[np.ndarray[int]],
        duration_matrices: list[np.ndarray[int]],
        groups: list[ClientGroup] = [],
    ) -> None: ...
    def location(self, idx: int) -> Client | Depot: ...
    def clients(self) -> list[Client]: ...
    def depots(self) -> list[Depot]: ...
    def groups(self) -> list[ClientGroup]: ...
    def vehicle_types(self) -> list[VehicleType]: ...
    def distance_matrices(self) -> list[np.ndarray[int]]: ...
    def duration_matrices(self) -> list[np.ndarray[int]]: ...
    def replace(
        self,
        clients: list[Client] | None = None,
        depots: list[Depot] | None = None,
        vehicle_types: list[VehicleType] | None = None,
        distance_matrices: list[np.ndarray[int]] | None = None,
        duration_matrices: list[np.ndarray[int]] | None = None,
        groups: list[ClientGroup] | None = None,
    ) -> ProblemData: ...
    def centroid(self) -> tuple[float, float]: ...
    def group(self, group: int) -> ClientGroup: ...
    def vehicle_type(self, vehicle_type: int) -> VehicleType: ...
    def distance_matrix(self, profile: int) -> np.ndarray[int]: ...
    def duration_matrix(self, profile: int) -> np.ndarray[int]: ...
    @property
    def num_clients(self) -> int: ...
    @property
    def num_groups(self) -> int: ...
    @property
    def num_depots(self) -> int: ...
    @property
    def num_locations(self) -> int: ...
    @property
    def num_vehicles(self) -> int: ...
    @property
    def num_vehicle_types(self) -> int: ...
    @property
    def num_profiles(self) -> int: ...
    @property
    def num_load_dimensions(self) -> int: ...
    def __eq__(self, other: object) -> bool: ...
    def __getstate__(self) -> tuple: ...
    def __setstate__(self, state: tuple, /) -> None: ...

class ScheduledVisit:
    start_service: int
    end_service: int
    wait_duration: int
    time_warp: int
    @property
    def service_duration(self) -> int: ...
    def __getstate__(self) -> tuple: ...
    def __setstate__(self, state: tuple, /) -> None: ...

class Route:
    def __init__(
        self, data: ProblemData, visits: list[int], vehicle_type: int
    ) -> None: ...
    def __getitem__(self, idx: int) -> int: ...
    def __iter__(self) -> Iterator[int]: ...
    def __len__(self) -> int: ...
    def __eq__(self, other: object) -> bool: ...
    def is_feasible(self) -> bool: ...
    def has_excess_load(self) -> bool: ...
    def has_excess_distance(self) -> bool: ...
    def has_time_warp(self) -> bool: ...
    def delivery(self) -> list[int]: ...
    def pickup(self) -> list[int]: ...
    def excess_load(self) -> list[int]: ...
    def excess_distance(self) -> int: ...
    def distance(self) -> int: ...
    def distance_cost(self) -> int: ...
    def duration(self) -> int: ...
    def duration_cost(self) -> int: ...
    def visits(self) -> list[int]: ...
    def time_warp(self) -> int: ...
    def start_time(self) -> int: ...
    def end_time(self) -> int: ...
    def slack(self) -> int: ...
    def service_duration(self) -> int: ...
    def travel_duration(self) -> int: ...
    def wait_duration(self) -> int: ...
    def release_time(self) -> int: ...
    def prizes(self) -> int: ...
    def centroid(self) -> tuple[float, float]: ...
    def vehicle_type(self) -> int: ...
    def start_depot(self) -> int: ...
    def end_depot(self) -> int: ...
    def schedule(self) -> list[ScheduledVisit]: ...
    def __getstate__(self) -> tuple: ...
    def __setstate__(self, state: tuple, /) -> None: ...

class Solution:
    def __init__(
        self,
        data: ProblemData,
        routes: list[Route] | list[list[int]],
    ) -> None: ...
    @classmethod
    def make_random(
        cls, data: ProblemData, rng: RandomNumberGenerator
    ) -> Solution: ...
    def neighbours(self) -> list[tuple[int, int] | None]: ...
    def routes(self) -> list[Route]: ...
    def has_excess_load(self) -> bool: ...
    def has_excess_distance(self) -> bool: ...
    def has_time_warp(self) -> bool: ...
    def distance(self) -> int: ...
    def distance_cost(self) -> int: ...
    def duration(self) -> int: ...
    def duration_cost(self) -> int: ...
    def excess_load(self) -> list[int]: ...
    def excess_distance(self) -> int: ...
    def fixed_vehicle_cost(self) -> int: ...
    def time_warp(self) -> int: ...
    def prizes(self) -> int: ...
    def uncollected_prizes(self) -> int: ...
    def is_feasible(self) -> bool: ...
    def is_group_feasible(self) -> bool: ...
    def is_complete(self) -> bool: ...
    def num_routes(self) -> int: ...
    def num_clients(self) -> int: ...
    def num_missing_clients(self) -> int: ...
    def __copy__(self) -> Solution: ...
    def __deepcopy__(self, memo: dict) -> Solution: ...
    def __hash__(self) -> int: ...
    def __eq__(self, other: object) -> bool: ...
    def __getstate__(self) -> tuple: ...
    def __setstate__(self, state: tuple, /) -> None: ...

class PopulationParams:
    generation_size: int
    lb_diversity: float
    min_pop_size: int
    nb_close: int
    nb_elite: int
    ub_diversity: float
    def __init__(
        self,
        min_pop_size: int = 25,
        generation_size: int = 40,
        nb_elite: int = 4,
        nb_close: int = 5,
        lb_diversity: float = 0.1,
        ub_diversity: float = 0.5,
    ) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    @property
    def max_pop_size(self) -> int: ...

class SubPopulation:
    def __init__(
        self,
        diversity_op: Callable[[Solution, Solution], float],
        params: PopulationParams,
    ) -> None: ...
    def add(
        self, solution: Solution, cost_evaluator: CostEvaluator
    ) -> None: ...
    def purge(self, cost_evaluator: CostEvaluator) -> None: ...
    def update_fitness(self, cost_evaluator: CostEvaluator) -> None: ...
    def __getitem__(self, idx: int) -> SubPopulationItem: ...
    def __iter__(self) -> Iterator[SubPopulationItem]: ...
    def __len__(self) -> int: ...

class SubPopulationItem:
    @property
    def fitness(self) -> float: ...
    @property
    def solution(self) -> Solution: ...
    def avg_distance_closest(self) -> float: ...

class DistanceSegment:
    def __init__(self, distance: int) -> None: ...
    @staticmethod
    def merge(
        edge_distance: int,
        first: DistanceSegment,
        second: DistanceSegment,
    ) -> DistanceSegment: ...
    def distance(self) -> int: ...

class LoadSegment:
    def __init__(self, delivery: int, pickup: int, load: int) -> None: ...
    @staticmethod
    def merge(first: LoadSegment, second: LoadSegment) -> LoadSegment: ...
    def delivery(self) -> int: ...
    def pickup(self) -> int: ...
    def load(self) -> int: ...

class DurationSegment:
    def __init__(
        self,
        duration: int,
        time_warp: int,
        tw_early: int,
        tw_late: int,
        release_time: int,
    ) -> None: ...
    @staticmethod
    def merge(
        edge_duration: int,
        first: DurationSegment,
        second: DurationSegment,
    ) -> DurationSegment: ...
    def duration(self) -> int: ...
    def tw_early(self) -> int: ...
    def tw_late(self) -> int: ...
    def time_warp(self, max_duration: int = ...) -> int: ...

class RandomNumberGenerator:
    @overload
    def __init__(self, seed: int) -> None: ...
    @overload
    def __init__(self, state: list[int]) -> None: ...
    @staticmethod
    def max() -> int: ...
    @staticmethod
    def min() -> int: ...
    def rand(self) -> float: ...
    def randint(self, high: int) -> int: ...
    def __call__(self) -> int: ...
    def state(self) -> list[int]: ...
