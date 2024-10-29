import pytest

from pyvrp import read, solve
from pyvrp.stop import MaxIterations


@pytest.mark.parametrize(
    "where",
    [
        "tests/data/RC208.vrp",  # VRPTW
        "tests/data/PR11A.vrp",  # Multi-depot VRP
        "tests/data/X-n101-50-k13.vrp",  # VRP with backhaul
    ],
)
def test_read(where: str, benchmark):
    """
    Tests runtime performance of solving various instances.
    """
    data = read(where, round_func="dimacs")
    benchmark(solve, data, stop=MaxIterations(1), seed=0)
