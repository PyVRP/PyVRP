import pathlib

import pytest

from pyvrp import read


def _read(where: str, *args, **kwargs):
    this_dir = pathlib.Path(__file__).parent
    return read(this_dir / where, *args, **kwargs)


@pytest.fixture(scope="session")
def ok_small():
    return _read("data/OkSmall.txt")


@pytest.fixture(scope="session")
def ok_small_prizes():
    return _read("data/OkSmallPrizes.txt")


@pytest.fixture(scope="session")
def rc208():
    return _read(
        "data/RC208.txt",
        instance_format="solomon",
        round_func="dimacs",
    )


@pytest.fixture(scope="session")
def prize_collecting():
    return _read("data/p06-2-50.vrp", round_func="dimacs")


@pytest.fixture(scope="session")
def small_cvrp():
    return _read("data/E-n22-k4.txt", round_func="trunc1")
