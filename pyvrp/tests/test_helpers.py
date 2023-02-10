from numpy.testing import assert_almost_equal, assert_raises

from pyvrp import PenaltyManager
from pyvrp.helpers import TooManyRoutesError, make_individual
from pyvrp.tests.helpers import read, read_solution


def test_make_individual():
    data = read(
        "data/RC208.txt", instance_format="solomon", round_func="trunc1"
    )
    solution = read_solution("data/RC208.sol")
    pm = PenaltyManager(data.vehicle_capacity)

    indiv = make_individual(data, pm, solution["routes"])
    assert_almost_equal(indiv.cost() / 10, solution["cost"])

    with assert_raises(TooManyRoutesError):
        indiv = make_individual(data, pm, solution["routes"] * 100)
