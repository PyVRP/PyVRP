from copy import copy, deepcopy

from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import Individual, ProblemData, XorShift128
from pyvrp.tests.helpers import read


def test_route_constructor_sorts_by_empty():
    data = read("data/OkSmall.txt")

    indiv = Individual(data, [[3, 4], [], [1, 2]])
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

    assert_equal(data.num_vehicles, 3)

    # Only two routes should not raise. But we should always get num_vehicles
    # routes back.
    individual = Individual(data, [[1, 2], [4, 2]])
    assert_equal(len(individual.get_routes()), data.num_vehicles)

    # Empty third route should not raise.
    Individual(data, [[1, 2], [4, 2], []])

    # More than three routes should raise, since we only have three vehicles.
    with assert_raises(RuntimeError):
        Individual(data, [[1], [2], [3], [4]])


def test_get_neighbours():
    data = read("data/OkSmall.txt")

    indiv = Individual(data, [[3, 4], [], [1, 2]])
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

    # This solution is infeasible due to both load and time window violations.
    indiv = Individual(data, [[1, 2, 3, 4]])
    assert_(not indiv.is_feasible())

    # First route has total load 18, but vehicle capacity is only 10.
    assert_(indiv.has_excess_load())

    # Client 4 has TW [8400, 15300], but client 2 cannot be visited before
    # 15600, so there must be time warp on the single-route solution.
    assert_(indiv.has_time_warp())

    # Let's try another solution that's actually feasible.
    indiv = Individual(data, [[1, 2], [3], [4]])
    assert_(indiv.is_feasible())
    assert_(not indiv.has_excess_load())
    assert_(not indiv.has_time_warp())


def test_distance_calculation():
    data = read("data/OkSmall.txt")

    indiv = Individual(data, [[1, 2], [3], [4]])
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
    assert_equal(indiv.distance(), dist)


def test_excess_load_calculation():
    data = read("data/OkSmall.txt")

    indiv = Individual(data, [[4, 3, 1, 2]])
    assert_(indiv.has_excess_load())
    assert_(not indiv.has_time_warp())

    # All clients are visited on the same route/by the same vehicle. The total
    # demand is 18, but the vehicle capacity is only 10. This has a non-zero
    # load penalty
    assert_equal(indiv.excess_load(), 18 - data.route(0).vehicle_capacity)


def test_time_warp_calculation():
    data = read("data/OkSmall.txt")

    indiv = Individual(data, [[1, 3], [2, 4]])
    assert_(not indiv.has_excess_load())
    assert_(indiv.has_time_warp())

    # There's only time warp on the first route: dist(0, 1) = 1'544, so we
    # arrive at 1 before its opening window of 15'600. Service (360) thus
    # starts at 15'600, and completes at 15'600 + 360. Then we drive dist(1, 3)
    # = 1'427, where we arrive after 15'300 (its closing time window). This is
    # where we incur time warp: we need to 'warp back' to 15'300.
    tw_first_route = 15_600 + 360 + 1_427 - 15_300
    tw_second_route = 0
    assert_equal(indiv.time_warp(), tw_first_route + tw_second_route)


def test_time_warp_for_a_very_constrained_problem():
    """
    This tests an artificial instance where the second client cannot be reached
    directly from the depot in a feasible solution, but only after the first
    client.
    """
    data = ProblemData(
        coords=[(0, 0), (1, 0), (2, 0)],
        demands=[0, 0, 0],
        vehicle_capacities=[0, 0],
        time_windows=[(0, 10), (0, 5), (0, 5)],
        service_durations=[0, 0, 0],
        duration_matrix=[
            [0, 1, 10],  # cannot get to 2 from depot within 2's time window
            [1, 0, 1],
            [1, 1, 0],
        ],
    )

    # This solution directly visits the second client from the depot, which is
    # not time window feasible.
    infeasible = Individual(data, [[1], [2]])
    assert_(infeasible.has_time_warp())
    assert_(not infeasible.has_excess_load())
    assert_(not infeasible.is_feasible())

    # But visiting the second client after the first is feasible.
    feasible = Individual(data, [[1, 2]])
    assert_(not feasible.has_time_warp())
    assert_(not feasible.has_excess_load())
    assert_(feasible.is_feasible())


# TODO test all time warp cases
def test_num_routes_calculation():
    # TODO test if empty routes are not consequtive
    assert_(False)


def test_copy():
    data = read("data/OkSmall.txt")

    indiv = Individual(data, [[1, 2, 3, 4]])
    copy_indiv = copy(indiv)
    deepcopy_indiv = deepcopy(indiv)

    # Copied individuals are equal to the original individual
    assert_(indiv == copy_indiv)
    assert_(indiv == deepcopy_indiv)

    # But they are not the same object
    assert_(indiv is not copy_indiv)
    assert_(indiv is not deepcopy_indiv)


def test_eq():
    data = read("data/OkSmall.txt")

    indiv1 = Individual(data, [[1, 2, 3, 4]])
    indiv2 = Individual(data, [[1, 2], [3], [4]])
    indiv3 = Individual(data, [[1, 2, 3, 4]])

    assert_(indiv1 == indiv1)  # individuals should be equal to themselves
    assert_(indiv2 == indiv2)
    assert_(indiv1 != indiv2)  # different routes, so should not be equal
    assert_(indiv1 == indiv3)  # same solution, different individual

    indiv4 = Individual(data, [[1, 2, 3], [], [4]])
    indiv5 = Individual(data, [[4], [1, 2, 3], []])

    assert_(indiv4 == indiv5)  # routes are the same, but in different order

    # And a few tests against things that are not Individuals, just to be sure
    # there's also a type check in there somewhere.
    assert_(indiv4 != 1)
    assert_(indiv4 != "abc")
    assert_(indiv5 != 5)
    assert_(indiv5 != "cd")


def test_same_routes_different_vehicle_not_eq():
    data = ProblemData(
        coords=[(0, 0), (1, 0), (2, 0)],
        demands=[2, 3, 4],
        vehicle_capacities=[10, 20],
        time_windows=[(0, 10), (0, 5), (0, 5)],
        service_durations=[0, 0, 0],
        duration_matrix=[
            [0, 1, 10],  # cannot get to 2 from depot within 2's time window
            [1, 0, 1],
            [1, 1, 0],
        ],
    )

    indiv1 = Individual(data, [[1, 2, 3], []])
    indiv2 = Individual(data, [[], [1, 2, 3]])

    assert_(indiv1 != indiv2)


def test_str_contains_essential_information():
    data = read("data/OkSmall.txt")
    rng = XorShift128(seed=2)

    for _ in range(5):  # let's do this a few times to really make sure
        individual = Individual.make_random(data, rng)
        str_representation = str(individual).splitlines()

        routes = individual.get_routes()
        num_routes = individual.num_routes()

        # There should be no more than num_routes lines (each detailing a
        # single route), and a final line containing the distance.
        assert_equal(len(str_representation), num_routes + 1)

        # The first num_routes lines should each contain a route, where each
        # route should contain every client that is in the route as returned
        # by get_routes().
        for route, str_route in zip(routes[:num_routes], str_representation):
            for client in route:
                assert_(str(client) in str_route)

        # Last line should contain the distance (cost).
        assert_(str(individual.distance()) in str_representation[-1])


def test_hash():
    data = read("data/OkSmall.txt")
    rng = XorShift128(seed=2)

    indiv1 = Individual.make_random(data, rng)
    indiv2 = Individual.make_random(data, rng)

    hash1 = hash(indiv1)
    hash2 = hash(indiv2)

    # Two random solutions. They're not the same, so the hashes should not be
    # the same either.
    assert_(indiv1 != indiv2)
    assert_(hash1 != hash2)

    indiv3 = deepcopy(indiv2)  # is a direct copy

    # These two are the same solution, so their hashes should be the same too.
    assert_equal(indiv2, indiv3)
    assert_equal(hash(indiv2), hash(indiv3))
