import pathlib
import time
from functools import lru_cache

from pyvrp import Individual
from pyvrp.read import read as _read
from pyvrp.read import read_solution as _read_solution


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


def make_random_initial_solutions(data, pm, rng, num_init):
    """
    Returns a list of a ``num_init`` random initial solutions.
    """
    return [Individual.make_random(data, pm, rng) for _ in range(num_init)]


class DummyTarget:
    """
    Dummy optimisation target that implements the OptimisationTarget protocol.
    """

    def __init__(self, cost: float):
        self._cost = cost

    def cost(self):
        return self._cost
