from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator, RandomNumberGenerator, Route, Solution, Trip
from pyvrp.search import ChangeVehicleType, LocalSearch
from pyvrp.search.neighbourhood import compute_neighbours


def test_change_vehicle_type_no_op_single_vehicle_type(ok_small):
    """
    Tests that calling ChangeVehicleType on a problem with only one vehicle
    type is a no-op, since there are no different vehicle types to change to.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    ls = LocalSearch(ok_small, rng, neighbours)
    ls.add_perturbation_operator(ChangeVehicleType(ok_small, 1))

    sol = Solution.make_random(ok_small, rng)
    cost_eval = CostEvaluator([1], 1, 0)

    # Should return the same solution since there's only one vehicle type
    result = ls.perturb(sol, cost_eval)
    assert_equal(result, sol)


def test_change_vehicle_type_no_op_empty_solution(ok_small_multi_depot):
    """
    Tests that calling ChangeVehicleType on an empty solution is a no-op,
    since there are no routes to change vehicle types for.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small_multi_depot)
    ls = LocalSearch(ok_small_multi_depot, rng, neighbours)
    ls.add_perturbation_operator(ChangeVehicleType(ok_small_multi_depot, 1))

    sol = Solution(ok_small_multi_depot, [])
    cost_eval = CostEvaluator([1], 1, 0)

    # Should return the same solution since there are no routes
    result = ls.perturb(sol, cost_eval)
    assert_equal(result, sol)


def test_change_vehicle_type_no_op_zero_perturbations(ok_small_multi_depot):
    """
    Tests that calling ChangeVehicleType with num_perturb=0 is a no-op.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small_multi_depot)
    ls = LocalSearch(ok_small_multi_depot, rng, neighbours)
    ls.add_perturbation_operator(ChangeVehicleType(ok_small_multi_depot, 0))

    sol = Solution.make_random(ok_small_multi_depot, rng)
    cost_eval = CostEvaluator([1], 1, 0)

    # Should return the same solution since num_perturb=0
    result = ls.perturb(sol, cost_eval)
    assert_equal(result, sol)


def test_change_vehicle_type_can_be_instantiated(ok_small_multi_depot):
    """
    Tests that ChangeVehicleType can be instantiated and added to LocalSearch
    without errors.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small_multi_depot)
    ls = LocalSearch(ok_small_multi_depot, rng, neighbours)

    op = ChangeVehicleType(ok_small_multi_depot, 1)
    ls.add_perturbation_operator(op)

    assert_equal(len(ls.perturbation_operators), 1)
    assert_equal(ls.perturbation_operators[0], op)


def test_change_vehicle_type_selects_empty_routes(ok_small_multi_depot):
    """
    Tests that ChangeVehicleType selects empty routes to schange, if available.
    """
    data = ok_small_multi_depot
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(data)
    ls = LocalSearch(data, rng, neighbours)

    op = ChangeVehicleType(data, 1)
    ls.add_perturbation_operator(op)

    routes = [
        Route(data, [2], vehicle_type=0),
        Route(data, [3], vehicle_type=1),
    ]
    sol = Solution(data, routes)
    cost_eval = CostEvaluator([1], 1, 0)
    perturbed = ls.perturb(sol, cost_eval)

    # TODO this is an annoying test because I dont know the route order.
    # There is one more vehicle type.
    # Check that one of the routes remain intact.
    # There are two vehicles of type 1. The second route will change to type 1,
    # while the route 1 also stays type 1. It will be selected over the
    # existing route.
    # The first route will not be swapped because there are no other vehicle
    # types.
    # If we increase num perturb, we will end up with only 0 vehicle types.
    assert_equal(perturbed.routes()[0].vehicle_type(), 0)
    assert_equal(perturbed.routes()[1].vehicle_type(), 0)


def test_change_vehicle_type_two_non_empty_routes(ok_small_multi_depot):
    """
    Tests that ChangeVehicleType correctly changes the vehicle type
    for two non-empty routes in a solution.
    """
    data = ok_small_multi_depot
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(data)
    ls = LocalSearch(data, rng, neighbours)

    op = ChangeVehicleType(data, 1)
    ls.add_perturbation_operator(op)

    routes = [
        Route(data, [2], vehicle_type=0),
        Route(data, [3], vehicle_type=0),
        Route(data, [4], vehicle_type=1),
    ]
    sol = Solution(data, routes)
    cost_eval = CostEvaluator([1], 1, 0)
    perturbed = ls.perturb(sol, cost_eval)

    # There are no empty routes, so two routes of different types will be
    # changed. It's not clear which one will be changed, so we check both.
    assert_equal(perturbed.routes()[0].vehicle_type(), 0)
    assert_equal(perturbed.routes()[1].vehicle_type(), 0)
    assert_equal(perturbed.routes()[2].vehicle_type(), 1)
    assert_equal(perturbed.routes()[0].visits(), [4])
    assert_equal(perturbed.routes()[1].visits(), [3])
    assert_equal(perturbed.routes()[2].visits(), [2])


def test_change_vehicle_type_multiple_trips_no_op(ok_small_multiple_trips):
    """
    Tests that ChangeVehicleType does not change vehicle types when there are
    multiple trips in a route.
    """
    data = ok_small_multiple_trips
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(data)
    ls = LocalSearch(data, rng, neighbours)
    ls.add_perturbation_operator(ChangeVehicleType(data, 1))

    routes = [
        Route(data, [Trip(data, [1], 0), Trip(data, [2], 0)], 0),
        Route(data, [Trip(data, [3], 0), Trip(data, [], 0)], 0),
        Route(data, [4], vehicle_type=1),
    ]
    sol = Solution(data, routes)
    cost_eval = CostEvaluator([1], 1, 0)
    perturbed = ls.perturb(sol, cost_eval)

    # No swap occurs between vehicle types are incompatible. The first route
    # will change to type 1, and the second route will change
    assert_equal(perturbed, sol)

    # But it's OK to swap if the route has just one trip, despite having a
    # vehicle type that is not compatible with the trips.
    routes = [
        Route(data, [Trip(data, [1], 0), Trip(data, [2], 0)], 0),
        Route(data, [3], 0),
        Route(data, [4], 1),
    ]
    sol = Solution(data, routes)
    perturbed = ls.perturb(sol, cost_eval)

    assert_(perturbed != sol)
