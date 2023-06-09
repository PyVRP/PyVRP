import pathlib
import time
from functools import lru_cache
from typing import List

from pyvrp import Individual, ProblemData
from pyvrp.read import read as _read
from pyvrp.read import read_solution as _read_solution


def make_heterogeneous(data: ProblemData, vehicle_capacities: List[int]):
    """
    Creates a new ProblemData instance by replacing the vehicle capacities
    data. All other data are kept identical.
    """
    clients = [data.client(i) for i in range(data.num_clients + 1)]
    return ProblemData(
        clients=clients,
        vehicle_capacities=vehicle_capacities,
        distance_matrix=[
            [data.dist(i, j) for j in range(data.num_clients + 1)]
            for i in range(data.num_clients + 1)
        ],
        duration_matrix=[
            [data.duration(i, j) for j in range(data.num_clients + 1)]
            for i in range(data.num_clients + 1)
        ],
    )


def get_route_visits(indiv: Individual):
    return [r.visits() for r in indiv.get_routes()]


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
    return [Individual.make_random(data, rng) for _ in range(num_sols)]
