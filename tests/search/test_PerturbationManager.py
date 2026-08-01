from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises

import pyvrp
from pyvrp import Activity, CostEvaluator, RandomNumberGenerator
from pyvrp.search import (
    PerturbationManager,
    PerturbationParams,
    compute_neighbours,
)
from pyvrp.search._search import SearchSpace, Solution


def test_raises_max_smaller_than_min():
    """
    Tests that the PerturbationParams's constructor raises when the minimum
    number of perturbations exceeds the maximum.
    """
    with assert_raises(ValueError):
        PerturbationParams(1, 0)  # min > max

    PerturbationParams(0, 0)  # but min == max should be fine


def test_raises_zero_max_routes():
    """
    Tests that at least one route must be available for route perturbation.
    """
    with assert_raises(ValueError):
        PerturbationParams(max_routes=0)


def test_eq():
    """
    Tests that PerturbationParams's ``__eq__`` implementation is correct.
    """
    params = PerturbationParams()
    assert_(params == PerturbationParams())
    assert_(params != PerturbationParams(1, 10))
    assert_(params != PerturbationParams(max_routes=1))

    assert_equal(params.max_routes, 3)

    assert_(params != "")
    assert_(params != 123)


def test_shuffle():
    """
    Tests shuffling and drawing random number of perturbations.
    """
    params = PerturbationParams(1, 10)
    manager = PerturbationManager(params)

    rng = RandomNumberGenerator(seed=42)
    for _ in range(10):  # all samples should be within bounds
        manager.shuffle(rng)
        assert_(1 <= manager.num_perturbations() <= 10)

    params = PerturbationParams(0, 0)
    manager = PerturbationManager(params)
    for _ in range(10):  # same, but now we can only draw one outcome: 0
        manager.shuffle(rng)
        assert_equal(manager.num_perturbations(), 0)


def test_num_perturbations_randomness():
    """
    Tests the bounds and randomness of repeated shuffles.
    """
    params = PerturbationParams(1, 10)
    manager = PerturbationManager(params)

    # Collect a large sample.
    rng = RandomNumberGenerator(seed=42)
    sample = []
    for _ in range(1_000):
        manager.shuffle(rng)
        sample.append(manager.num_perturbations())

    # We should have drawn uniformly from [min, max] perturbations. The mean
    # number of perturbations should be min + (max - min) / 2, with some
    # allowance for randomness.
    min_perturbs = params.min_perturbations
    max_perturbs = params.max_perturbations
    avg_perturbs = min_perturbs + (max_perturbs - min_perturbs) / 2
    assert_equal(min(sample), min_perturbs)
    assert_equal(max(sample), max_perturbs)
    assert_allclose(sum(sample) / len(sample), avg_perturbs, atol=0.05)


def test_route_perturb_selects_neighbouring_routes(ok_small):
    """
    Tests that route perturbation selects routes from the neighbourhood.
    """
    neighbours = {
        Activity("C0"): [Activity("C2")],
        Activity("C1"): [],
        Activity("C2"): [],
        Activity("C3"): [],
    }
    cost_eval = CostEvaluator([0], 0, 0)

    cases = [
        (1, [1, 2]),
        (2, [1, 3]),
    ]
    for max_routes, expected in cases:
        sol = Solution(ok_small)
        sol.load(pyvrp.Solution(ok_small, [[0, 3], [1], [2]]))
        search_space = SearchSpace(ok_small, neighbours)

        params = PerturbationParams(
            2,
            2,
            max_routes=max_routes,
        )
        perturbation = PerturbationManager(params)

        # This seed selects route perturbation and, when possible, two routes.
        perturbation.shuffle(RandomNumberGenerator(seed=1))
        perturbation.perturb(sol, search_space, cost_eval)

        planned = sorted(
            activity.idx
            for route in sol.unload().routes()
            for activity in route
            if activity.is_client()
        )
        assert_equal(planned, expected)


def test_route_perturb_inserts_neighbours_into_seed_route(ok_small):
    """
    Tests that an unplanned seed and its neighbours enter the same route.
    """
    sol = Solution(ok_small)
    neighbours = {
        Activity("C0"): [Activity("C1")],
        Activity("C1"): [],
        Activity("C2"): [],
        Activity("C3"): [],
    }
    search_space = SearchSpace(ok_small, neighbours)
    cost_eval = CostEvaluator([0], 0, 0)

    params = PerturbationParams(2, 2, max_routes=1)
    perturbation = PerturbationManager(params)
    perturbation.shuffle(RandomNumberGenerator(seed=1))
    perturbation.perturb(sol, search_space, cost_eval)

    perturbed = sol.unload()
    assert_equal(perturbed.num_routes(), 1)
    assert_equal(perturbed.num_clients(), 2)
    assert_equal(
        sorted(
            activity.idx
            for activity in perturbed.routes()[0]
            if activity.is_client()
        ),
        [0, 1],
    )


def test_route_perturb_uses_multiple_unplanned_seeds(ok_small):
    """
    Tests that route perturbation handles multiple unplanned seed nodes.
    """
    sol = Solution(ok_small)
    neighbours = {
        Activity("C0"): [Activity("C1"), Activity("C2")],
        Activity("C1"): [Activity("C3")],
        Activity("C2"): [],
        Activity("C3"): [],
    }
    search_space = SearchSpace(ok_small, neighbours)
    cost_eval = CostEvaluator([0], 0, 0)

    params = PerturbationParams(4, 4, max_routes=2)
    perturbation = PerturbationManager(params)
    perturbation.shuffle(RandomNumberGenerator(seed=1))
    perturbation.perturb(sol, search_space, cost_eval)

    planned = sorted(
        activity.idx
        for route in sol.unload().routes()
        for activity in route
        if activity.is_client()
    )
    assert_equal(planned, [0, 1, 2, 3])


def test_route_perturb_mixes_removal_and_insertion(ok_small):
    """
    Tests that planned and unplanned seed nodes are handled independently.
    """
    sol = Solution(ok_small)
    sol.load(pyvrp.Solution(ok_small, [[0, 3]]))
    neighbours = {
        Activity("C0"): [Activity("C1")],
        Activity("C1"): [Activity("C2")],
        Activity("C2"): [],
        Activity("C3"): [],
    }
    search_space = SearchSpace(ok_small, neighbours)
    cost_eval = CostEvaluator([0], 0, 0)

    params = PerturbationParams(4, 4, max_routes=2)
    perturbation = PerturbationManager(params)
    perturbation.shuffle(RandomNumberGenerator(seed=1))
    perturbation.perturb(sol, search_space, cost_eval)

    planned = sorted(
        activity.idx
        for route in sol.unload().routes()
        for activity in route
        if activity.is_client()
    )
    assert_equal(planned, [1, 2])


def test_route_perturb_marks_inserted_shipment_promising(small_shipments):
    """
    Tests that both activities of an inserted seed shipment are promising.
    """
    sol = Solution(small_shipments)
    search_space = SearchSpace(
        small_shipments,
        compute_neighbours(small_shipments),
    )
    cost_eval = CostEvaluator([0], 0, 0)

    params = PerturbationParams(1, 1, max_routes=1)
    perturbation = PerturbationManager(params)
    perturbation.shuffle(RandomNumberGenerator(seed=1))
    perturbation.perturb(sol, search_space, cost_eval)

    assert_(search_space.is_promising(Activity("L0")))
    assert_(search_space.is_promising(Activity("U0")))


def test_perturb_inserts_clients(ok_small):
    """
    Tests that perturbing an empty solution inserts all missing clients.
    """
    sol = Solution(ok_small)  # start empty

    search_space = SearchSpace(ok_small, compute_neighbours(ok_small))
    cost_eval = CostEvaluator([20], 6, 0)

    # Perturb the empty solution exactly four times. That means we should
    # insert all missing clients.
    perturbation = PerturbationManager(PerturbationParams(4, 4))
    perturbation.perturb(sol, search_space, cost_eval)

    perturbed = sol.unload()
    assert_equal(perturbed.num_clients(), 4)


def test_perturb_removes_clients(ok_small):
    """
    Tests that perturbing a complete solution could remove all clients.
    """
    sol = Solution(ok_small)  # load a complete solution
    sol.load(pyvrp.Solution(ok_small, [[0, 1], [2, 3]]))

    search_space = SearchSpace(ok_small, compute_neighbours(ok_small))
    cost_eval = CostEvaluator([20], 6, 0)

    # Perturb the complete solution four times. That means we should remove all
    # clients, and the perturbed solution should be empty.
    perturbation = PerturbationManager(PerturbationParams(4, 4))
    perturbation.perturb(sol, search_space, cost_eval)

    perturbed = sol.unload()
    assert_equal(perturbed.num_clients(), 0)


def test_perturb_switches_remove_insert(ok_small):
    """
    Tests that perturbing switches between inserting and removing, depending
    on whether a random initial client is in the solution.
    """
    sol = Solution(ok_small)  # start with [C0, C1] in the solution
    sol.load(pyvrp.Solution(ok_small, [[0, 1]]))

    # We want to perturb three times. We begin by perturbing C0. Since C0 is in
    # the solution, we remove. C1 is in C0's neighbourhood, so we also remove
    # C1. Then we move to perturb C1, but it's already been perturbed and none
    # of its neighbourhood members are in the solution, so there is nothing we
    # can do. So we move to perturb C2: it's not in the solution, has not been
    # perturbed yet, so we insert it. That's the third and final perturbation,
    # so the perturbed solution should contain only C2.
    search_space = SearchSpace(ok_small, compute_neighbours(ok_small))
    cost_eval = CostEvaluator([0], 0, 0)

    perturbation = PerturbationManager(PerturbationParams(3, 3))
    perturbation.perturb(sol, search_space, cost_eval)

    # Test that the perturbed solution contains only C2.
    perturbed = sol.unload()
    clients = [
        activity.idx
        for route in perturbed.routes()
        for activity in route
        if activity.is_client()
    ]

    assert_equal(perturbed.num_clients(), 1)
    assert_equal(clients, [2])


def test_perturb_inserts_into_new_routes(ok_small):
    """
    Tests that we can perturb into empty routes.
    """
    # Change capacity so that each route can serve exactly one client.
    veh_type = ok_small.vehicle_type(0)
    data = ok_small.replace(vehicle_types=[veh_type.replace(capacity=[5])])

    # Start with an empty solution, and an empty granular neighbourhood. So
    # there is no way to insert clients next to their neighbours.
    sol = Solution(data)
    neighbours = {Activity(f"C{idx}"): [] for idx in range(data.num_clients)}
    search_space = SearchSpace(data, neighbours)
    cost_eval = CostEvaluator([2000], 0, 0)  # heavily penalise load violations

    # Perturb exactly three times. No clients are currently in the solution, so
    # we insert. There are no neighbours, so we insert into empty routes (or
    # the first route, which is a default). The large load violation penalty
    # ensures that the empty routes are always better. We should thus end up
    # with three routes in the perturbed solution.
    perturbation = PerturbationManager(PerturbationParams(3, 3))
    perturbation.perturb(sol, search_space, cost_eval)

    perturbed = sol.unload()
    assert_equal(perturbed.num_routes(), 3)


def test_perturb_shipments_remove(small_shipments):
    """
    Tests perturbing an instance with shipments where the perturbation involves
    removing shipments.
    """
    data = small_shipments
    rng = RandomNumberGenerator(seed=42)

    rnd_sol = pyvrp.Solution.make_random(data, rng)
    assert_(rnd_sol.is_complete())
    assert_equal(rnd_sol.num_shipments(), 4)

    sol = Solution(data)
    sol.load(rnd_sol)

    search_space = SearchSpace(data, compute_neighbours(data))
    cost_eval = CostEvaluator([1], 1, 0)

    # The random solution is complete, so all we can do is remove shipments.
    # We should remove only one, because we haven't shuffled yet and we default
    # to min_perturbations in that case.
    perturbation = PerturbationManager()
    assert_equal(perturbation.num_perturbations(), 1)

    perturbation.perturb(sol, search_space, cost_eval)
    perturbed = sol.unload()

    assert_(not perturbed.is_complete())
    assert_equal(perturbed.num_shipments(), 3)


def test_perturb_shipments_insert(small_shipments):
    """
    Tests perturbing a shipment solution once, by inserting the first missing
    shipment into the non-empty solution.
    """
    activities = [Activity("L1"), Activity("U1")]
    route = pyvrp.Route(small_shipments, activities, 0)
    pyvrp_sol = pyvrp.Solution(small_shipments, [route])

    sol = Solution(small_shipments)
    sol.load(pyvrp_sol)

    params = PerturbationParams(1, 1)  # perturb exactly once
    perturbation = PerturbationManager(params)

    neighbours = compute_neighbours(small_shipments)
    search_space = SearchSpace(small_shipments, neighbours)
    cost_eval = CostEvaluator([1], 1, 0)
    perturbation.perturb(sol, search_space, cost_eval)

    # We should have inserted the first missing shipment into the solution.
    # Since nothing was shuffled, that first missing shipment is L0/U0.
    perturbed = sol.unload()
    assert_equal(perturbed.num_shipments(), 2)
    assert_(Activity("L0") not in perturbed.unplanned())
    assert_(Activity("L2") in perturbed.unplanned())


def test_perturb_shipment_empty_route(small_shipments):
    """
    Tests perturbing an empty shipment solution, so shipments must be inserted
    into empty routes.
    """
    sol = Solution(small_shipments)
    empty = sol.unload()
    assert_equal(empty.num_shipments(), 0)
    assert_(not empty.is_complete())

    # Perturbation should insert all missing shipments into the empty solution.
    params = PerturbationParams(min_perturbations=4)
    perturbation = PerturbationManager(params)

    neighbours = compute_neighbours(small_shipments)
    search_space = SearchSpace(small_shipments, neighbours)
    cost_eval = CostEvaluator([1], 1, 0)
    perturbation.perturb(sol, search_space, cost_eval)

    # And thus, after perturbation we expect a complete solution.
    perturbed = sol.unload()
    assert_equal(perturbed.num_shipments(), 4)
    assert_(perturbed.is_complete())
