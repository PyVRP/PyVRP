import pytest

from tests.helpers import read


@pytest.fixture(scope="session")
def vrptw():
    return read("data/RC208.vrp", round_func="dimacs")


@pytest.fixture(scope="session")
def mdvrp():
    return read("data/PR11A.vrp", round_func="trunc")


@pytest.fixture(scope="session")
def vrpb():
    return read("data/X-n101-50-k13.vrp", round_func="round")
