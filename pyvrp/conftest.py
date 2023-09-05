import pytest

from pyvrp.tests.helpers import read


@pytest.fixture(scope="session")
def ok_small():
    """
    Fixture that returns the OkSmall instance when called, an instance with
    just 4 clients that's easy to understand.
    """
    return read("data/OkSmall.txt")


@pytest.fixture(scope="session")
def ok_small_prizes():
    """
    Fixture that returns the OkSmall instance when called, adapted for prize
    collecting and optional customers.
    """
    return read("data/OkSmallPrizes.txt")


@pytest.fixture(scope="session")
def rc208():
    """
    Fixture that returns the RC208 VRPTW instance when called, with 100
    clients.
    """
    return read(
        "data/RC208.txt",
        instance_format="solomon",
        round_func="dimacs",
    )


@pytest.fixture(scope="session")
def prize_collecting():
    """
    Fixture that returns a prize-collecting instance with 50 clients when
    called.
    """
    return read("data/p06-2-50.vrp", round_func="dimacs")


@pytest.fixture(scope="session")
def small_cvrp():
    """
    Fixture that returns a small CVRP instance with just 21 clients.
    """
    return read("data/E-n22-k4.txt", round_func="dimacs")
