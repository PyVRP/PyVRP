from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises

import pyvrp
from pyvrp import Activity, Client, CostEvaluator, RandomNumberGenerator
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


def test_eq():
    """
    Tests that PerturbationParams's ``__eq__`` implementation is correct.
    """
    params = PerturbationParams()
    assert_(params == PerturbationParams())
    assert_(params != PerturbationParams(1, 10))

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


def test_perturb_groups_neighbours_by_route(small_shipments):
    """
    Tests that perturbation processes neighbours by route.
    """
    data = small_shipments

    # Shipments 0 and 1 share the same route, shipment 3 is on another route,
    # and shipment 2 starts unplanned.
    activities1 = [Activity(des) for des in ["L0", "U0", "L1", "U1"]]
    activities2 = [Activity(des) for des in ["L3", "U3"]]
    route1 = pyvrp.Route(data, activities1, 0)
    route2 = pyvrp.Route(data, activities2, 0)

    sol = Solution(data)
    sol.load(pyvrp.Solution(data, [route1, route2]))

    # We will perturb shipment 0. Its neighbours are shipments 2, 3 and 1,
    # in that order.
    neighbours = {Activity(f"L{idx}"): [] for idx in range(data.num_shipments)}
    neighbours[Activity("L0")] = [
        Activity("U2"),
        Activity("U3"),
        Activity("U1"),
    ]
    search_space = SearchSpace(data, neighbours)
    cost_eval = CostEvaluator([1], 1, 0)

    perturbation = PerturbationManager(PerturbationParams(6, 6))
    perturbation.perturb(sol, search_space, cost_eval)

    # Removing shipments 0 and 1 from the same route uses four perturbations.
    # Inserting shipment 2 uses the last two, exhausting the budget before
    # shipment 3 on the other route is considered.
    perturbed = sol.unload()
    unplanned = {
        activity.idx
        for activity in perturbed.unplanned()
        if activity.is_pickup()
    }

    assert_equal(perturbed.num_shipments(), 2)
    assert_equal(unplanned, {0, 1})


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

    # One perturbation is not enough to remove a shipment.
    perturbation = PerturbationManager(PerturbationParams(1, 1))
    perturbation.perturb(sol, search_space, cost_eval)

    unperturbed = sol.unload()
    assert_(unperturbed.is_complete())
    assert_equal(unperturbed.num_shipments(), 4)

    # Two perturbations remove exactly one shipment.
    perturbation = PerturbationManager(PerturbationParams(2, 2))

    perturbation.perturb(sol, search_space, cost_eval)
    perturbed = sol.unload()

    assert_(not perturbed.is_complete())
    assert_equal(perturbed.num_shipments(), 3)
    assert_equal({activity.idx for activity in perturbed.unplanned()}, {0})


def test_perturb_shipments_insert(small_shipments):
    """
    Tests perturbing an instance with shipments where the perturbation involves
    inserting shipments.
    """
    activities = [Activity("L1"), Activity("U1")]
    route = pyvrp.Route(small_shipments, activities, 0)
    pyvrp_sol = pyvrp.Solution(small_shipments, [route])

    sol = Solution(small_shipments)
    sol.load(pyvrp_sol)

    neighbours = compute_neighbours(small_shipments)
    search_space = SearchSpace(small_shipments, neighbours)
    cost_eval = CostEvaluator([1], 1, 0)

    # One perturbation is not enough to insert a shipment.
    search_space.mark_all_promising()
    perturbation = PerturbationManager(PerturbationParams(1, 1))
    perturbation.perturb(sol, search_space, cost_eval)

    unperturbed = sol.unload()
    assert_equal(unperturbed.num_shipments(), 1)
    assert_(Activity("L0") in unperturbed.unplanned())

    # Two perturbations insert exactly one shipment.
    perturbation = PerturbationManager(PerturbationParams(2, 2))
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

    # Each shipment requires two perturbations, so eight perturbations should
    # insert all missing shipments into the empty solution.
    params = PerturbationParams(min_perturbations=8)
    perturbation = PerturbationManager(params)

    neighbours = compute_neighbours(small_shipments)
    search_space = SearchSpace(small_shipments, neighbours)
    cost_eval = CostEvaluator([1], 1, 0)
    perturbation.perturb(sol, search_space, cost_eval)

    # And thus, after perturbation we expect a complete solution.
    perturbed = sol.unload()
    assert_equal(perturbed.num_shipments(), 4)
    assert_(perturbed.is_complete())


def test_perturb_uses_remaining_move_on_client(small_shipments):
    """
    Tests that a single, remaining perturbation move is spent perturbing a
    client, because a shipment requires two moves.
    """
    data = small_shipments.replace(clients=[Client(2, delivery=[0])])
    sol = Solution(data)

    neighbours = compute_neighbours(data)
    search_space = SearchSpace(data, neighbours)

    # Ensure that a shipment is considered before the client.
    search_space.shuffle(RandomNumberGenerator(seed=42))
    assert_(search_space.activity_order()[0].is_pickup())

    cost_eval = CostEvaluator([1], 1, 0)
    perturbation = PerturbationManager(PerturbationParams(1, 1))
    perturbation.perturb(sol, search_space, cost_eval)

    # Shipments are never perturbed because they require two moves. The client
    # uses the single available move instead.
    perturbed = sol.unload()
    assert_equal(perturbed.num_clients(), 1)
    assert_equal(perturbed.num_shipments(), 0)
