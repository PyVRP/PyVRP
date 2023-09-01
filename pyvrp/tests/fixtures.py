import pytest

from pyvrp.tests.helpers import read


@pytest.fixture(scope="session")
def ok_small():
    return read("data/OkSmall.txt")


@pytest.fixture(scope="session")
def ok_small_prizes():
    return read("data/OkSmallPrizes.txt")


@pytest.fixture(scope="session")
def rc208():
    return read(
        "data/RC208.txt",
        instance_format="solomon",
        round_func="dimacs",
    )


@pytest.fixture(scope="session")
def prize_collecting():
    return read("data/p06-2-50.vrp", round_func="dimacs")


@pytest.fixture(scope="session")
def small_cvrp():
    return read("data/E-n22-k4.txt", round_func="trunc1")
