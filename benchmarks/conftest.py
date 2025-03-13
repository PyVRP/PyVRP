import pytest

from tests.helpers import read, read_solution


@pytest.fixture(scope="session")
def vrptw():
    return read("data/RC208.vrp", round_func="dimacs")


@pytest.fixture(scope="session")
def vrptw_bks(vrptw):
    return read_solution("data/RC208.sol", vrptw)


@pytest.fixture(scope="session")
def mdvrp():
    return read("data/PR11A.vrp", round_func="trunc")


@pytest.fixture(scope="session")
def vrpb():
    return read("data/X-n101-50-k13.vrp", round_func="round")
