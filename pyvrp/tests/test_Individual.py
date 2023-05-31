from copy import copy, deepcopy

import numpy as np
from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises
from pytest import mark

from pyvrp import Client, Individual, ProblemData, XorShift128
from pyvrp.tests.helpers import make_heterogeneous, read


def test_route_constructor_sorts_by_empty():
    data = read("data/OkSmall.txt")

    indiv = Individual(data, [[3, 4], [], [1, 2]])
    routes = indiv.get_routes()

    # num_non_empty_routes() should show two non-empty routes. However, we
    # passed in three routes, so len(routes) should not have changed.
    assert_equal(indiv.num_non_empty_routes(), 2)
    assert_equal(len(routes), 3)

    # We expect Individual to sort the routes such that all non-empty routes
    # are in the lower indices.
    assert_equal(len(routes[0]), 2)
    assert_equal(len(routes[1]), 2)
    assert_equal(len(routes[2]), 0)

    # Test heterogeneous case
    data = make_heterogeneous(data, [10, 10, 10, 20, 20])
    indiv = Individual(data, [[], [3, 4], [], [], [1, 2]])

    # num_non_empty_routes() should show two non-empty routes.
    assert_equal(indiv.num_non_empty_routes(), 2)

    # We expect Individual to sort the routes such that all non-empty routes
    # are in the lower indices for each group of equal vehicle capacities.
    assert_equal(indiv.get_routes(), [[3, 4], [], [], [1, 2], []])


def test_random_constructor_cycles_over_routes():
    # This instance has four clients and three vehicles. Since 1 client per
    # vehicle would not work (insufficient vehicles), each route is given two
    # clients (and the last route should be empty).
    data = read("data/OkSmall.txt")
    rng = XorShift128(seed=42)

    indiv = Individual.make_random(data, rng)
    routes = indiv.get_routes()

    assert_equal(indiv.num_routes(), 2)
    assert_equal(len(routes), 3)

    for idx, size in enumerate([2, 2, 0]):
        assert_equal(len(routes[idx]), size)


def test_route_constructor_raises_too_many_vehicles():
    data = read("data/OkSmall.txt")

    assert_equal(data.max_num_routes, 3)

    # Only two routes should not raise. But we should always get max_num_routes
    # routes back.
    individual = Individual(data, [[1, 2], [4, 3]])
    assert_equal(len(individual.get_routes()), data.max_num_routes)

    # Empty third route should not raise.
    Individual(data, [[1, 2], [4, 3], []])

    # More than three routes should raise, since we only have three vehicles.
    with assert_raises(RuntimeError):
        Individual(data, [[1], [2], [3], [4]])


def test_route_constructor_raises_for_invalid_routes():
    data = read("data/OkSmall.txt")
    with assert_raises(RuntimeError):
        Individual(data, [[1, 2], [1, 3, 4]])  # client 1 is visited twice

    data = read("data/OkSmallPrizes.txt")
    with assert_raises(RuntimeError):
        Individual(data, [[2], [3, 4]])  # 1 is required but not visited


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
    routes = indiv.get_routes()

    # Solution is feasible, so all its routes should also be feasible.
    assert_(indiv.is_feasible())
    assert_(all(route.is_feasible() for route in routes))

    # Solution distance should be equal to all routes' distances. These we
    # check separately.
    assert_allclose(
        indiv.distance(), sum(route.distance() for route in routes)
    )

    expected = data.dist(0, 1) + data.dist(1, 2) + data.dist(2, 0)
    assert_allclose(routes[0].distance(), expected)

    expected = data.dist(0, 3) + data.dist(3, 0)
    assert_allclose(routes[1].distance(), expected)

    expected = data.dist(0, 4) + data.dist(4, 0)
    assert_allclose(routes[2].distance(), expected)


def test_excess_load_calculation():
    data = read("data/OkSmall.txt")

    indiv = Individual(data, [[4, 3, 1, 2]])
    assert_(indiv.has_excess_load())
    assert_(not indiv.has_time_warp())

    # All clients are visited on the same route/by the same vehicle. The total
    # demand is 18, but the vehicle capacity is only 10.
    assert_equal(indiv.excess_load(), 18 - data.route_data(0).vehicle_capacity)


def test_heterogeneous_capacity_excess_load_calculation():
    data = read("data/OkSmall.txt")
    data = make_heterogeneous(data, vehicle_capacities=[10, 10, 20])

    # This instance has capacities [10, 10, 20] and total demand of 18 so if
    # all demand is put in the first route the excess_load is 18 - 10 = 8.
    indiv = Individual(data, [[1, 2, 3, 4]])
    assert_(indiv.has_excess_load())
    assert_equal(indiv.excess_load(), 8)

    # Same for the second route
    indiv = Individual(data, [[], [1, 2, 3, 4]])
    assert_(indiv.has_excess_load())
    assert_equal(indiv.excess_load(), 8)

    # Third route has larger capacity than demand, so there is no excess load.
    indiv = Individual(data, [[], [], [1, 2, 3, 4]])
    assert_(not indiv.has_excess_load())
    assert_equal(indiv.excess_load(), 0)


def test_route_access_methods():
    data = read("data/OkSmall.txt")
    indiv = Individual(data, [[1, 3], [2, 4]])
    routes = indiv.get_routes()

    # Test route access: getting the route plan should return a simple list, as
    # given to the individual above.
    assert_equal(routes[0].visits(), [1, 3])
    assert_equal(routes[1].visits(), [2, 4])

    # There's no excess load, so all excess load should be zero.
    assert_(not indiv.has_excess_load())
    assert_allclose(routes[0].excess_load(), 0)
    assert_allclose(routes[1].excess_load(), 0)

    # Total route demand.
    demands = [data.client(idx).demand for idx in range(data.num_clients + 1)]
    assert_allclose(routes[0].demand(), demands[1] + demands[3])
    assert_allclose(routes[1].demand(), demands[2] + demands[4])

    # The first route is not feasible due to time warp, but the second one is.
    # See also the tests below.
    assert_(not routes[0].is_feasible())
    assert_(routes[1].is_feasible())

    # Total service duration.
    services = [
        data.client(idx).service_duration
        for idx in range(data.num_clients + 1)
    ]
    assert_allclose(routes[0].service_duration(), services[1] + services[3])
    assert_allclose(routes[1].service_duration(), services[2] + services[4])


def test_route_time_warp_and_wait_duration():
    data = read("data/OkSmall.txt")
    indiv = Individual(data, [[1, 3], [2, 4]])
    routes = indiv.get_routes()

    # There's only time warp on the first route: duration(0, 1) = 1'544, so we
    # arrive at 1 before its opening window of 15'600. Service (360) thus
    # starts at 15'600, and completes at 15'600 + 360. Then we drive for
    # duration(1, 3) = 1'427, where we arrive after 15'300 (its closing time
    # window). This is where we incur time warp: we need to 'warp' to 15'300.
    assert_(indiv.has_time_warp())
    assert_(routes[0].has_time_warp())
    assert_(not routes[1].has_time_warp())
    assert_allclose(routes[0].time_warp(), 15_600 + 360 + 1_427 - 15_300)
    assert_allclose(routes[1].time_warp(), 0)
    assert_allclose(indiv.time_warp(), routes[0].time_warp())

    # On the first route, we have to wait at the first client, where we arrive
    # at 1'544, but cannot start service until 15'600. On the second route, we
    # also have to wait at the first client, where we arrive at 1'944, but
    # cannot start service until 12'000.
    assert_equal(routes[0].wait_duration(), 15_600 - 1_544)
    assert_equal(routes[1].wait_duration(), 12_000 - 1_944)


@mark.parametrize(
    "dist_mat",
    [
        np.full((3, 3), fill_value=100, dtype=int),
        np.full((3, 3), fill_value=1, dtype=int),
        np.full((3, 3), fill_value=1000, dtype=int),
    ],
)
def test_time_warp_for_a_very_constrained_problem(dist_mat):
    """
    This tests an artificial instance where the second client cannot be reached
    directly from the depot in a feasible solution, but only after the first
    client.
    """
    dur_mat = [
        [0, 1, 10],  # cannot get to 2 from depot within 2's time window
        [1, 0, 1],
        [1, 1, 0],
    ]

    data = ProblemData(
        clients=[
            Client(x=0, y=0, tw_late=10),
            Client(x=1, y=0, tw_late=5),
            Client(x=2, y=0, tw_late=5),
        ],
        vehicle_capacities=[0, 0],
        vehicle_cap=0,
        distance_matrix=dist_mat,
        duration_matrix=dur_mat,
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

    assert_equal(
        feasible.distance(),
        dist_mat[0, 1] + dist_mat[1, 2] + dist_mat[2, 0],
    )


# TODO test all time warp cases


def test_num_non_empty_routes_calculation():
    data = read("data/OkSmall.txt")
    data = make_heterogeneous(data, vehicle_capacities=[10, 10, 20])

    indiv = Individual(data, [[1, 2, 3, 4]])
    assert_equal(indiv.num_non_empty_routes(), 1)

    indiv = Individual(data, [[], [1, 2, 3, 4]])
    assert_equal(indiv.num_non_empty_routes(), 1)

    indiv = Individual(data, [[], [], [1, 2, 3, 4]])
    assert_equal(indiv.num_non_empty_routes(), 1)

    indiv = Individual(data, [[1, 2], [3, 4]])
    assert_equal(indiv.num_non_empty_routes(), 2)

    indiv = Individual(data, [[1, 2], [], [3, 4]])
    assert_equal(indiv.num_non_empty_routes(), 2)

    indiv = Individual(data, [[1], [2], [3, 4]])
    assert_equal(indiv.num_non_empty_routes(), 3)


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
    """
    Tests that two individuals are not considered equal if they have the same
    routes (orders of clients) but served by vehicles with different
    capacities.
    """
    data = read("data/OkSmall.txt")
    # Make sure capacities are different but large enough (>18) to have no
    # violations so have the same attributes, such that we actually test if the
    # assignments are used for the equality comparison.
    data = make_heterogeneous(data, vehicle_capacities=[20, 20, 30])

    indiv1 = Individual(data, [[1, 2, 3, 4]])
    indiv2 = Individual(data, [[], [1, 2, 3, 4]])
    indiv3 = Individual(data, [[], [], [1, 2, 3, 4]])

    # First two vehicles have same capacity, so order does not matter
    assert_(indiv1 == indiv2)
    # Third vehicle has different capacity, so this solution is different
    assert_(indiv1 != indiv3)


def test_heterogeneous_route_sorting():
    """
    Tests that individual sorts non-empty routes per group of same capacities.
    """
    data = read("data/OkSmall.txt")
    data = make_heterogeneous(data, vehicle_capacities=[10, 10, 20])

    indiv1 = Individual(data, [[1, 2, 3, 4]])
    indiv2 = Individual(data, [[], [1, 2, 3, 4]])
    indiv3 = Individual(data, [[], [], [1, 2, 3, 4]])

    # First two vehicles have same capacity, so order does not matter
    expected = [[1, 2, 3, 4], [], []]
    assert_equal(indiv1.get_routes(), expected)
    assert_equal(indiv2.get_routes(), expected)

    # Third vehicle has a different capacity, so should not be moved forward
    assert_equal(indiv3.get_routes(), [[], [], [1, 2, 3, 4]])


def test_unsorted_heterogeneous_route_sorting():
    """
    Tests that if routes/vehicles with the same capacities are not sorted in
    ProblemData, their routes also won't be sorted. Hence, we don't require
    routes to be sorted but it is more efficient to have them sorted.
    """
    data = read("data/OkSmall.txt")
    data = make_heterogeneous(data, vehicle_capacities=[10, 20, 10])

    indiv1 = Individual(data, [[1, 2, 3, 4]])
    indiv2 = Individual(data, [[], [1, 2, 3, 4]])
    indiv3 = Individual(data, [[], [], [1, 2, 3, 4]])

    # First two vehicles have different capacities, so order does matter
    assert_equal(indiv1.get_routes(), [[1, 2, 3, 4], [], []])
    assert_equal(indiv2.get_routes(), [[], [1, 2, 3, 4], []])

    # Third vehicle has a different capacity than the second, so should not be
    # moved forward even though it has the same capacity as the first vehicle.
    assert_equal(indiv3.get_routes(), [[], [], [1, 2, 3, 4]])


@mark.parametrize(
    "capacities",
    [[10, 10, 10], [10, 10, 20]],
)
def test_str_contains_essential_information(capacities):
    data = read("data/OkSmall.txt")
    data = make_heterogeneous(data, vehicle_capacities=capacities)

    rng = XorShift128(seed=2)

    for _ in range(5):  # let's do this a few times to really make sure
        individual = Individual.make_random(data, rng)
        str_representation = str(individual).splitlines()

        routes = individual.get_routes()
        num_non_empty_routes = individual.num_non_empty_routes()

        # There should be no more than num_non_empty_routes lines (each
        # detailing a single route), and two final lines containing distance
        # and prizes.
        assert_equal(len(str_representation), num_non_empty_routes + 2)

        # The first num_non_empty_routes lines should each contain a route,
        # where each route should contain every client that is in the route as
        # returned by get_routes().
        for route, str_route in zip(
            routes[:num_non_empty_routes], str_representation
        ):
            for client in route:
                assert_(str(client) in str_route)

        # Last lines should contain the travel distance and collected prizes.
        assert_(str(individual.distance()) in str_representation[-2])
        assert_(str(individual.prizes()) in str_representation[-1])


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
