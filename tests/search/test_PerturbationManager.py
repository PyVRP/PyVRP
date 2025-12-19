from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises

import pyvrp
from pyvrp import CostEvaluator, RandomNumberGenerator
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
    Tests that PerturbationParams's ``__eq__`` implementation.
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
    sol.load(pyvrp.Solution(ok_small, [[1, 2], [3, 4]]))

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
    sol = Solution(ok_small)  # start with [1, 2] in the solution
    sol.load(pyvrp.Solution(ok_small, [[1, 2]]))

    #  We want to perturb three times. We begin by perturbing 1. Since 1 is in
    # the solution, we remove. As 2 is in 1's neighbourhood, so we also remove
    # 2. Then we move to perturb 2, but it's already been perturbed and has an
    # empty neighbourhood, so there is nothing we can do. So we move to perturb
    # 3: it's not in the solution, has not been perturbed yet, so we insert it.
    # That's the third and final perturbation, so the perturbed solution should
    # contain only client 3.
    search_space = SearchSpace(ok_small, compute_neighbours(ok_small))
    cost_eval = CostEvaluator([0], 0, 0)

    perturbation = PerturbationManager(PerturbationParams(3, 3))
    perturbation.perturb(sol, search_space, cost_eval)

    # Test that the perturbed solution contains only client 3.
    perturbed = sol.unload()
    visits = [visit for r in perturbed.routes() for visit in r.visits()]
    assert_equal(visits, [3])


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
    neighbours = [[] for _ in range(data.num_locations)]

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
