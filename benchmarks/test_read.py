from itertools import product

import pytest

from pyvrp import read


@pytest.mark.parametrize(
    ("where", "round_func"),
    product(
        [
            "tests/data/RC208.vrp",  # VRPTW
            "tests/data/PR11A.vrp",  # Multi-depot VRP
            "tests/data/X-n101-50-k13.vrp",  # VRP with backhaul
        ],
        ["none", "dimacs"],
    ),
)
@pytest.mark.benchmark
def test_read(where: str, round_func: str):
    """
    Tests runtime performance of reading various instances.
    """
    read(where, round_func=round_func)
