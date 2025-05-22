from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.accept import MovingAverageThreshold


@mark.parametrize(
    "eta, gamma",
    [
        (-1, 1),  # eta cannot be < 0
        (1, -2),  # gamma cannot be < 0
        (1, 0),  # gamma cannot be 0
    ],
)
def test_raise_invalid_parameters(eta: float, gamma: int):
    """
    Tests that a ValueError is raised when invalid parameters are passed.
    """
    with assert_raises(ValueError):
        MovingAverageThreshold(eta=eta, gamma=gamma)


@mark.parametrize("eta, gamma", [(1, 3), (0.4, 4)])
def test_no_raise_valid_parameters(eta, gamma):
    """
    Tests that nothing is raised when valid parameters are passed.
    """
    MovingAverageThreshold(eta=eta, gamma=gamma)


@mark.parametrize("eta", [0, 0.01, 0.5, 0.99, 1])
def test_eta(eta):
    """
    Teststhat the eta parameter is correctly set.
    """
    mat = MovingAverageThreshold(eta, 3)
    assert_equal(mat.eta, eta)


@mark.parametrize("gamma", range(1, 3))
def test_gamma(gamma):
    """
    Tests that the gamma parameter is correctly set.
    """
    mat = MovingAverageThreshold(0.5, gamma)
    assert_equal(mat.gamma, gamma)


def test_accepts_below_threshold():
    """
    Tests that MAT accepts a candidate solution below the threshold.
    """
    mat = MovingAverageThreshold(eta=0.5, gamma=4)
    mat(1, 1, 1)
    mat(1, 1, 0)

    # The threshold is set at 0 + 0.5 * (0.5 - 0) = 0.25, candidate has cost 0.
    assert_(mat(1, 1, 0))


def test_rejects_above_threshold():
    """
    Tests that MAT rejects a candidate solution above the threshold.
    """
    mat = MovingAverageThreshold(eta=0.5, gamma=4)
    mat(1, 1, 2)
    mat(1, 1, 0)

    # The threshold is set at 0 + 0.5 * (1 - 0) = 0, candidate has cost 1.
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
    mat = MovingAverageThreshold(eta=0.5, gamma=2)

    # First candidate is always accepted.
    assert_(mat(1, 1, 2))

    # Second candidate is rejected, because the threshold is set at
    # 2 + 0.5 * (3 - 2) = 2.5.
    assert_(not mat(1, 1, 4))

    # Third candidate is accepted, because the threshold is set at
    # 1 + 0.5 * (2.5 - 1) = 1.75.
    assert_(mat(1, 1, 1))

    # Fourth candidate is rejected, because the threshold is set at
    # 1 + 0.5 * (1.5 - 1) = 1.25.
    assert_(not mat(1, 1, 2))
