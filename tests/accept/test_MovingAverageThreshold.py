from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.accept import MovingAverageThreshold


@mark.parametrize(
    "eta, gamma, max_runtime",
    [
        (-1, 1, 0),  # eta cannot be < 0
        (1, -2, 0),  # gamma cannot be < 0
        (1, 0, 0),  # gamma cannot be 0
        (1, 1, -1),  # max_runtime cannot be < 0
    ],
)
def test_raise_invalid_parameters(eta: float, gamma: int, max_runtime: float):
    """
    Tests that a ValueError is raised when invalid parameters are passed.
    """
    with assert_raises(ValueError):
        MovingAverageThreshold(eta=eta, gamma=gamma, max_runtime=max_runtime)


@mark.parametrize(
    "eta, gamma, max_runtime",
    [
        (1, 3, 0),
        (0.4, 4, 10),
    ],
)
def test_attributes_are_correctly_set(
    eta: float, gamma: int, max_runtime: int
):
    """
    Tests that valid parameters are correctly set.
    """
    mat = MovingAverageThreshold(eta=eta, gamma=gamma, max_runtime=max_runtime)

    assert_equal(mat.eta, eta)
    assert_equal(mat.gamma, gamma)
    assert_equal(mat.max_runtime, max_runtime)


def test_accepts_below_threshold():
    """
    Tests that MAT accepts a candidate solution below the threshold.
    """
    mat = MovingAverageThreshold(eta=0.5, gamma=4)
    mat(1, 1, 1)
    mat(1, 1, 2)

    # The threshold is set at 0 + 0.5 * (1 - 0) = 0.5, candidate has cost 0.
    assert_(mat(1, 1, 0))


def test_rejects_above_threshold():
    """
    Tests that MAT rejects a candidate solution above the threshold.
    """
    mat = MovingAverageThreshold(eta=0.5, gamma=4)
    mat(1, 1, 2)
    mat(1, 1, 0)

    # The threshold is set at 0 + 0.5 * (1 - 0) = 0.5, candidate has cost 1.
    assert_(not mat(1, 1, 1))


def test_accepts_equal_threshold():
    """
    Tests that MAT accepts a candidate solution equal to the threshold.
    """
    mat = MovingAverageThreshold(eta=0.5, gamma=4)
    mat(1, 1, 1)
    mat(1, 1, 1)

    # The threshold is set at 1 + 0.5 * (1 - 1) = 1, candidate has cost 1.
    assert_(mat(1, 1, 1))


def test_threshold_updates_with_history():
    """
    Tests that MAT correctly updates the threshold based on the history of
    candidate solutions and also forgets older solutions.
    """
    mat = MovingAverageThreshold(eta=1, gamma=2)

    # First candidate is always accepted.
    assert_(mat(1, 1, 2))

    # Threshold is set at 2 + 0.5 * (3 - 2) = 2.5, so reject.
    assert_(not mat(1, 1, 4))

    # Threshold is set at 1 + 0.5 * (2.5 - 1) = 1.75, so accept.
    assert_(mat(1, 1, 1))

    # Threshold is set at 1 + 0.5 * (1.5 - 1) = 1.25, so reject.
    assert_(not mat(1, 1, 2))


def test_threshold_converges_with_max_runtime():
    """
    Tests that MAT correctly converges the threshold based on the runtime.
    """
    mat = MovingAverageThreshold(eta=0.5, gamma=10, max_runtime=0)

    mat(1, 1, 10)
    mat(1, 1, 0)

    # Even though the candidate solutions are (10, 0, 2), the threshold
    # is set to 0 because the max runtime has elapsed.
    assert_(not mat(1, 1, 2))
