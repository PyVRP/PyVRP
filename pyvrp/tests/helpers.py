import pathlib
import time
from functools import lru_cache
from typing import List

from pyvrp import Individual, ProblemData
from pyvrp.read import read as _read
from pyvrp.read import read_solution as _read_solution


def make_heterogeneous(data: ProblemData, vehicle_capacities: List[int]):
    """
    Function to convert ProblemData to heteregeneous vehicle capacity instance.
    """
    clients = [data.client(i) for i in range(data.num_clients + 1)]
    return ProblemData(
        coords=[(c.x, c.y) for c in clients],
        demands=[c.demand for c in clients],
        vehicle_capacities=vehicle_capacities,
        time_windows=[(c.tw_early, c.tw_late) for c in clients],
        service_durations=[c.service_duration for c in clients],
        duration_matrix=[
            [data.dist(i, j) for j in range(data.num_clients + 1)]
            for i in range(data.num_clients + 1)
        ],
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
    return [Individual.make_random(data, rng) for _ in range(num_sols)]
