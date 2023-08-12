import pathlib
import time
from functools import lru_cache
from typing import List

from pyvrp import ProblemData, VehicleType
from pyvrp.read import read as _read
from pyvrp.read import read_solution as _read_solution


def make_heterogeneous(data: ProblemData, vehicle_types: List[VehicleType]):
    """
    Creates a new ProblemData instance by replacing the capacities for routes.
    All other data are kept identical.
    """
    clients = [data.client(i) for i in range(data.num_clients + 1)]
    return ProblemData(
        clients=clients,
        vehicle_types=vehicle_types,
        distance_matrix=data.distance_matrix(),
        duration_matrix=data.duration_matrix(),
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
