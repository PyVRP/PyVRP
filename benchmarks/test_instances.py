from itertools import product

import pytest

from pyvrp import read, solve
from pyvrp.stop import MaxIterations


@pytest.mark.parametrize(
    ("instance", "seed"),
    product(
        [
            "tests/data/RC208.vrp",  # VRPTW
            "tests/data/PR11A.vrp",  # Multi-depot VRP
            "tests/data/X-n101-50-k13.vrp",  # VRP with backhaul
        ],
        [0, 42, 3407],
    ),
)
def test_single_iteration_instance_performance(
    instance: str,
    seed: int,
    benchmark,
):
    """
    Tests runtime performance of single-iteration calls to solve(), with
    several different seeds and instances.
    """
    data = read(instance)
    benchmark(solve, data, stop=MaxIterations(1), seed=seed)
