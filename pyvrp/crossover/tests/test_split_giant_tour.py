from numpy.testing import assert_equal

from pyvrp.crossover.split_giant_tour import split_giant_tour
from pyvrp.tests.helpers import read


def test_split_giant_tour():
    # TODO find better name
    data = read("data/OkSmall.txt")

    # The tour is ordered in increasing order of earliest time window and
    # created, so a single route is feasible.
    tour = [3, 2, 1, 4]
    vehicle_types = [0, 0, 0]
    routes = split_giant_tour(tour, vehicle_types, data)

    assert_equal(len(routes), 1)
    assert_equal(routes, [[3, 2, 1, 4]])

    # The earliest time window of client 4 is later than the other time
    # windows, so we need to split the tour into two routes.
    tour = [4, 3, 2, 1]
    vehicle_types = [0, 0, 0]
    routes = split_giant_tour(tour, vehicle_types, data)

    assert_equal(len(routes), 2)
    assert_equal(routes, [[4], [3, 2, 1]])


def test_put_all_clients_to_last_vehicle():
    # TODO Single route

    # TODO two routes, but three needed
    pass


def test_ignore_feasibility_last_vehicle():
    """
    Tests that all remaining clients are added to the last vehicle, regardless
    whether the resulting route is feasible or not.
    """
    data = read("data/OkSmall.txt")
    tour = [4, 1, 3, 2]

    # We only have one vehicle type, so we can only create one route.
    vehicle_types = [0]
    routes = split_giant_tour(tour, vehicle_types, data)
    assert_equal(len(routes), 1)
    assert_equal(routes, [[4, 1, 3, 2]])

    # 3 -> 2 is not feasible, but the maximum number of routes is 3.
    assert_equal(routes, [[4], [1], [3, 2]])
