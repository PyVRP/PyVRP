import pathlib
import time
from functools import lru_cache
from typing import List, Optional

from pyvrp import Client, ProblemData, Solution, VehicleType
from pyvrp.read import read as _read
from pyvrp.read import read_solution as _read_solution


def customize(
    data: ProblemData,
    clients: Optional[List[Client]] = None,
    vehicle_types: Optional[List[VehicleType]] = None,
    duration_matrix: Optional[List[List[int]]] = None,
    distance_matrix: Optional[List[List[int]]] = None,
) -> ProblemData:
    """
    Returns a customized version of the given ``ProblemData`` object.
    """
    if clients is None:
        clients = [data.client(idx) for idx in range(data.num_clients + 1)]

    if vehicle_types is None:
        vehicle_types = [
            data.vehicle_type(idx) for idx in range(data.num_vehicle_types)
        ]

    if duration_matrix is None:
        duration_matrix = [
            [data.duration(i, j) for j in range(data.num_clients + 1)]
            for i in range(data.num_clients + 1)
        ]

    if distance_matrix is None:
        distance_matrix = [
            [data.dist(i, j) for j in range(data.num_clients + 1)]
            for i in range(data.num_clients + 1)
        ]

    return ProblemData(
        clients=clients,
        vehicle_types=vehicle_types,
        duration_matrix=duration_matrix,
        distance_matrix=distance_matrix,
    )


@lru_cache
def read(where: str, *args, **kwargs):
    """
    Lightweight wrapper around ``pyvrp.read.read()``, reading problem files
    relative to the current directory.
    """
    this_dir = pathlib.Path(__file__).parent
    return _read(this_dir / where, *args, **kwargs)


@lru_cache
def read_solution(where: str, *args, **kwargs):
    """
    Lightweight wrapper around ``pyvrp.read.read_solution()``, reading solution
    files relative to the current directory.
    """
    this_dir = pathlib.Path(__file__).parent
    return _read_solution(this_dir / where, *args, **kwargs)


def sleep(duration, get_now=time.perf_counter):
    """
    Custom sleep function. Built-in ``time.sleep()`` is not precise/depends on
    the OS, see https://stackoverflow.com/q/1133857/4316405.
    """
    now = get_now()
    end = now + duration
    while now < end:
        now = get_now()


def make_random_solutions(num_sols, data, rng):
    """
    Returns a list of ``num_sols`` random solutions.
    """
    return [Solution.make_random(data, rng) for _ in range(num_sols)]
