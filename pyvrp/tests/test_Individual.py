from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import Individual, PenaltyManager
from pyvrp.tests.helpers import read


def test_route_constructor_sorts_by_empty():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)

    indiv = Individual(data, pm, [[3, 4], [], [1, 2]])
    routes = indiv.get_routes()

    # num_routes() should show two non-empty routes. However, we passed in
    # three routes, so len(routes) should not have changed.
    assert_equal(indiv.num_routes(), 2)
    assert_equal(len(routes), 3)

    # We expect Individual to sort the routes such that all non-empty routes
    # are in the lower indices.
    assert_equal(len(routes[0]), 2)
    assert_equal(len(routes[1]), 2)
    assert_equal(len(routes[2]), 0)


def test_route_constructor_raises():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)

    assert_equal(data.num_vehicles, 3)

    with assert_raises(RuntimeError):
        # Two routes, but three vehicles: should raise.
        Individual(data, pm, [[1, 2], [4, 2]])

    # Empty third route should not raise.
    Individual(data, pm, [[1, 2], [4, 2], []])


def test_get_neighbours():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)

    indiv = Individual(data, pm, [[3, 4], [], [1, 2]])
    neighbours = indiv.get_neighbours()

    expected = [
        (0, 0),  # 0: is depot
        (0, 2),  # 1: between depot (0) to 2
        (1, 0),  # 2: between 1 and depot (0)
        (0, 4),  # 3: between depot (0) and 4
        (3, 0),  # 4: between 3 and depot (0)
    ]

    assert_equal(data.num_clients, 4)

    for client in range(data.num_clients + 1):  # incl. depot
        assert_equal(neighbours[client], expected[client])


def test_feasibility():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)

    # This solution is infeasible due to both load and time window violations.
    indiv = Individual(data, pm, [[1, 2, 3, 4], [], []])
    assert_(not indiv.is_feasible())

    # First route has total load 18, but vehicle capacity is only 10.
    assert_(indiv.has_excess_capacity())

    # Client 4 has TW [8400, 15300], but client 2 cannot be visited before
    # 15600, so there must be time warp on the single-route solution.
    assert_(indiv.has_time_warp())

    # Let's try another solution that's actually feasible.
    indiv = Individual(data, pm, [[1, 2], [3], [4]])
    assert_(indiv.is_feasible())
    assert_(not indiv.has_excess_capacity())
    assert_(not indiv.has_time_warp())


def test_distance_cost_calculation():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)

    indiv = Individual(data, pm, [[1, 2], [3], [4]])
    assert_(indiv.is_feasible())

    # Feasible individual, so cost should equal total distance travelled.
    dist = (
        data.dist(0, 1)
        + data.dist(1, 2)
        + data.dist(2, 0)
        + data.dist(0, 3)
        + data.dist(3, 0)
        + data.dist(0, 4)
        + data.dist(4, 0)
    )

    assert_equal(indiv.cost(), dist)


def test_capacity_cost_calculation():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)

    indiv = Individual(data, pm, [[4, 3, 1, 2], [], []])
    assert_(indiv.has_excess_capacity())
    assert_(not indiv.has_time_warp())

    # All clients are visited on the same route/by the same vehicle. The total
    # demand is 18, but the vehicle capacity is only 10. This has a non-zero
    # load penalty
    load_penalty = pm.load_penalty(18)
    assert_(load_penalty > 0)

    # The total costs are now load_penalty + dist
    dist = (
        data.dist(0, 4)
        + data.dist(4, 3)
        + data.dist(3, 1)
        + data.dist(1, 2)
        + data.dist(2, 0)
    )

    assert_equal(indiv.cost(), dist + load_penalty)


def test_time_warp_cost_calculation():
    data = read("data/OkSmall.txt")
    pm = PenaltyManager(data.vehicle_capacity)

    indiv = Individual(data, pm, [[1, 3], [2, 4], []])
    assert_(not indiv.has_excess_capacity())
    assert_(indiv.has_time_warp())

    # There's only time warp on the first route: dist(0, 1) = 1'544, so we
    # arrive at 1 before its opening window of 15'600. Service (360) thus
    # starts at 15'600, and completes at 15'600 + 360. Then we drive dist(1, 3)
    # = 1'427, where we arrive after 15'300 (its closing time window). This is
    # where we incur time warp: we need to 'warp back' to 15'300.
    tw_first_route = 15_600 + 360 + 1_427 - 15_300
    tw_second_route = 0
    tw_penalty = pm.tw_penalty(tw_first_route + tw_second_route)

    # The total costs are now tw_penalty + dist
    dist = (
        data.dist(0, 1)
        + data.dist(1, 3)
        + data.dist(3, 0)
        + data.dist(0, 2)
        + data.dist(2, 4)
        + data.dist(4, 0)
    )

    assert_equal(indiv.cost(), dist + tw_penalty)


# TODO test all time warp cases
