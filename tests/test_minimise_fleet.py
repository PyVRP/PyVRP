from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import minimise_fleet
from pyvrp.stop import MaxIterations
from tests.helpers import read


def test_raises_multiple_vehicle_types(ok_small_multi_depot):
    """
    Tests that ``minimise_fleet`` raises when given an instance with multiple
    vehicle types.
    """
    assert_equal(ok_small_multi_depot.num_vehicle_types, 2)

    with assert_raises(ValueError):
        minimise_fleet(ok_small_multi_depot, MaxIterations(1))


def test_raises_optional_clients(prize_collecting):
    """
    Tests that ``minimise_fleet`` raises when given an instance with optional
    clients. Fleet minimisation only works for complete problems.
    """
    with assert_raises(ValueError):
        minimise_fleet(prize_collecting, MaxIterations(1))


def test_OkSmall(ok_small):
    """
    Tests that the fleet minimisation procedure attains the lower bound on the
    OkSmall instance.
    """
    assert_equal(ok_small.num_vehicles, 3)

    vehicle_type = minimise_fleet(ok_small, MaxIterations(10))
    data = ok_small.replace(vehicle_types=[vehicle_type])
    assert_equal(data.num_vehicles, 2)


def test_OkSmall_multidimensional_load(ok_small_multiple_load):
    """
    Tests that the fleet minimisation procedure attains the lower bound on the
    OkSmall instance when considering multiple load dimensions.
    """
    assert_equal(ok_small_multiple_load.num_load_dimensions, 2)
    assert_equal(ok_small_multiple_load.num_vehicle_types, 1)

    # Need at least three because the client demand in the second dimension
    # sums to 5, yet each vehicle can only carry 2.
    vehicle_type = minimise_fleet(ok_small_multiple_load, MaxIterations(10))
    assert_equal(vehicle_type.num_available, 3)


def test_rc208(rc208):
    """
    Tests that the fleet minimisation procedure significantly reduces the
    number of vehicles in the RC208 instance.
    """
    assert_equal(rc208.num_vehicles, 25)

    vehicle_type = minimise_fleet(rc208, MaxIterations(3))
    data = rc208.replace(vehicle_types=[vehicle_type])
    assert_(data.num_vehicles < rc208.num_vehicles)


def test_X_instance():
    """
    Tests that the fleet minimisation procedure attains the lower bound on this
    particular X instance.
    """
    data = read("data/X-n101-50-k13.vrp", round_func="round")
    assert_equal(data.num_vehicles, 100)

    vehicle_type = minimise_fleet(data, MaxIterations(10))
    data = data.replace(vehicle_types=[vehicle_type])
    assert_equal(data.num_vehicles, 13)


def test_lower_bound_multi_trip_instance(ok_small_multiple_trips):
    """
    Tests that the fleet minimisation procedure adjusts the lower bound based
    on the vehicle's multi-trip capabilities.
    """
    new_veh_type = ok_small_multiple_trips.vehicle_type(0)
    data = ok_small_multiple_trips.replace(vehicle_types=[new_veh_type])

    # The vehicle capacity is insufficient to visit every client in a single
    # trip, but this problem can be solved in a single route, as 3 4 | 1 2.
    vehicle_type = minimise_fleet(data, MaxIterations(10))
    assert_equal(vehicle_type.num_available, 1)
