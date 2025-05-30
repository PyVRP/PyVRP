from numpy.testing import assert_, assert_equal

from pyvrp import CostEvaluator, RandomNumberGenerator, Solution, VehicleType
from pyvrp.perturb import DestroyRepair, GreedyRepair
from pyvrp.search.neighbourhood import compute_neighbours


def test_greedy_repair_results_in_complete_solution(ok_small):
    """
    Tests that greedy repairing an empty solution results in a complete
    solution with all clients inserted.
    """
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    cost_eval = CostEvaluator([1], 1, 0)
    sol = Solution(ok_small, [])

    assert_equal(sol.num_clients(), 0)
    assert_equal(sol.num_missing_clients(), 4)

    dr = DestroyRepair(ok_small)
    dr.add_repair_operator(GreedyRepair(ok_small))
    perturbed = dr(sol, cost_eval, neighbours, rng)

    assert_equal(perturbed.num_clients(), 4)
    assert_equal(perturbed.num_missing_clients(), 0)


def test_greedy_repair_inserts_into_empty_route(ok_small):
    """
    Tests that greedy repair inserts clients into empty routes.
    """
    cost_eval = CostEvaluator([1000], 1000, 1)
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(ok_small)
    sol = Solution(ok_small, [[1, 2, 3]])

    dr = DestroyRepair(ok_small)
    dr.add_repair_operator(GreedyRepair(ok_small))
    perturbed = dr(sol, cost_eval, neighbours, rng)
    new_routes = [route.visits() for route in perturbed.routes()]

    # It's best to insert client 4 into an empty route to avoid penalties.
    assert_equal(len(new_routes), 2)
    assert_equal(new_routes, [[1, 2, 3], [4]])


def test_greedy_repair_inserts_into_empty_route_multiple_vehicle_types(
    ok_small,
):
    """
    Tests that greedy repair inserts clients into empty routes, selecting
    vehicle types at random.
    """
    cost_eval = CostEvaluator([1000], 1000, 1)
    data = ok_small.replace(
        vehicle_types=[
            VehicleType(num_available=2, capacity=[5], fixed_cost=0),
            VehicleType(num_available=1, capacity=[5], fixed_cost=100),
        ]
    )
    rng = RandomNumberGenerator(seed=42)
    neighbours = compute_neighbours(data)
    sol = Solution(data, [[1, 2, 3]])

    dr = DestroyRepair(data)
    dr.add_repair_operator(GreedyRepair(data))

    # It's best to insert client 4 into an empty route to avoid penalties.
    # The vehicle type of the empty route is selected at random, so we
    # repeatedly perturb the same solution here, and check which vehicle
    # type was used to insert the new client.
    vehicle_types = []
    for _ in range(10):
        perturbed = dr(sol, cost_eval, neighbours, rng)
        assert_equal(perturbed.num_routes(), 2)

        vehicle_type = perturbed.routes()[1].vehicle_type()
        vehicle_types.append(vehicle_type)

    # Check if both vehicle types were used over the different perturbations.
    assert_(0 in vehicle_types)
    assert_(1 in vehicle_types)
