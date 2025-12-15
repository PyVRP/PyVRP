from numpy.testing import assert_, assert_allclose, assert_equal, assert_raises

from pyvrp import RandomNumberGenerator
from pyvrp.search import PerturbationManager, PerturbationParams


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
    Tests shuffling and drawing random number of permutations.
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
    # number of permutations should be min + (max - min) / 2, with some
    # allowance for randomness.
    min_perturbs = params.min_perturbations
    max_perturbs = params.max_perturbations
    avg_perturbs = min_perturbs + (max_perturbs - min_perturbs) / 2
    assert_equal(min(sample), min_perturbs)
    assert_equal(max(sample), max_perturbs)
    assert_allclose(sum(sample) / len(sample), avg_perturbs, atol=0.05)
