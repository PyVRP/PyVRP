from numpy.testing import assert_equal

from pyvrp.crossover.split_giant_tour import split_giant_tour
from pyvrp.tests.helpers import read


def test_split_giant_tour():
    data = read("data/OkSmallSplitTour.txt")

    # The tour is ordered in increasing order of earliest time window and
    # created such a route is feasible.
    tour = [3, 2, 1, 4]
    routes = split_giant_tour(tour, data)
    assert_equal(len(routes), 1)
    assert_equal(routes, [[3, 2, 1, 4]])

    # The earliest time window of client 4 is later than the other time
    # windows, so we need to split the tour into two routes.
    tour = [4, 3, 2, 1]
    routes = split_giant_tour(tour, data)

    assert_equal(len(routes), 2)
    assert_equal(routes, [[4], [3, 2, 1]])


def test_split_giant_tour_ignore_feasibility_last_vehicle():
    """
    Tests that all remaining clients are added to the last vehicle if the
    number of routes is equal to the number of vehicles, regardless of
    whether the resulting route is feasible or not.
    """
    data = read("data/OkSmallSplitTour.txt")
    tour = [4, 1, 3, 2]
    routes = split_giant_tour(tour, data)

    assert_equal(len(routes), 3)

    # 3 -> 2 is not feasible, but the maximum number of routes is 3.
    assert_equal(routes, [[4], [1], [3, 2]])
