from numpy.testing import assert_raises

from pyvrp.search import PerturbationManager


def test_raises_max_smaller_than_min():
    """
    Tests that the PerturbationManager's constructor raises when the minimum
    number of perturbations exceeds the maximum.
    """
    with assert_raises(ValueError):
        PerturbationManager(1, 0)  # min > max

    PerturbationManager(0, 0)  # but min == max should be fine
