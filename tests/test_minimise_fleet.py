from numpy.testing import assert_equal

from pyvrp import minimise_fleet
from pyvrp.stop import MaxIterations
from tests.helpers import read


def test_OkSmall(ok_small):
    """
    Tests that the fleet minimisation procedure attains the lower bound on the
    OkSmall instance.
    """
    assert_equal(ok_small.num_vehicles, 3)

    veh_types = minimise_fleet(ok_small, MaxIterations(10))
    data = ok_small.replace(vehicle_types=veh_types)
    assert_equal(data.num_vehicles, 2)


def test_rc208(rc208):
    """
    Tests that the fleet minimisation procedure significantly reduces the
    number of vehicles in the RC208 instance.
    """
    assert_equal(rc208.num_vehicles, 25)

    veh_types = minimise_fleet(rc208, MaxIterations(10))
    data = rc208.replace(vehicle_types=veh_types)
    assert_equal(data.num_vehicles, 5)


def test_X_instance():
    """
    Tests that the fleet minimisation procedure significantly reduces the
    number of vehicles in this particular X instance.
    """
    data = read("data/X-n101-50-k13.vrp", round_func="round")
    assert_equal(data.num_vehicles, 100)

    veh_types = minimise_fleet(data, MaxIterations(10))
    data = data.replace(vehicle_types=veh_types)
    assert_equal(data.num_vehicles, 14)
