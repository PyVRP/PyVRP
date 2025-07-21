from numpy.testing import assert_, assert_equal

from pyvrp import (
    CostEvaluator,
    Model,
    RandomNumberGenerator,
    Route,
    Solution,
    Trip,
)
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
    ls.add_perturbation_operator(ChangeVehicleType(ok_small))

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
    ls.add_perturbation_operator(ChangeVehicleType(ok_small_multi_depot))

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
    ls.add_perturbation_operator(ChangeVehicleType(ok_small_multi_depot))

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

    op = ChangeVehicleType(ok_small_multi_depot)
    ls.add_perturbation_operator(op)

    assert_equal(len(ls.perturbation_operators), 1)
    assert_equal(ls.perturbation_operators[0], op)


def test_change_vehicle_type_multiple_trips_no_op(ok_small_multiple_trips):
    """
    Tests that ChangeVehicleType does not change vehicle types when there are
    multiple trips in a route.
    """
    data = ok_small_multiple_trips
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(data)
    ls = LocalSearch(data, rng, neighbours)
    ls.add_perturbation_operator(ChangeVehicleType(data))

    route = Route(data, [Trip(data, [1], 0), Trip(data, [2], 0)], 0)
    sol = Solution(data, [route])
    cost_eval = CostEvaluator([1], 1, 0)
    perturbed = ls.perturb(sol, cost_eval)

    # No swap occurs because the route has multiple trips.
    assert_equal(perturbed, sol)


def test_supports():
    """
    Tests that ChangeVehicleType does not support instances with uniform fixed
    costs.
    """
    model = Model()
    model.add_depot(0, 0)
    model.add_vehicle_type(1, 1, fixed_cost=1)
    assert_(not ChangeVehicleType.supports(model.data()))

    model.add_vehicle_type(2, 1, fixed_cost=2)
    assert_(ChangeVehicleType.supports(model.data()))
