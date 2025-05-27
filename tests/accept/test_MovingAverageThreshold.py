from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.accept import MovingAverageThreshold


@mark.parametrize(
    "eta, gamma, max_runtime, max_iterations",
    [
        (-1, 1, 0, 0),  # eta cannot be < 0
        (1, -2, 0, 0),  # gamma cannot be < 0
        (1, 0, 0, 0),  # gamma cannot be 0
        (1, 1, -1, 0),  # max_runtime cannot be < 0
        (1, 1, 0, -1),  # max_iterations cannot be < 0
    ],
)
def test_raise_invalid_parameters(
    eta: float, gamma: int, max_runtime: float, max_iterations: int
):
    """
    Tests that a ValueError is raised when invalid parameters are passed.
    """
    with assert_raises(ValueError):
        MovingAverageThreshold(
            eta=eta,
            gamma=gamma,
            max_runtime=max_runtime,
            max_iterations=max_iterations,
        )


@mark.parametrize(
    "eta, gamma, max_runtime, max_iterations",
    [
        (1, 2, 3, 4),
        (0.4, 4, 10.1, 100),
    ],
)
def test_attributes_are_correctly_set(
    eta: float,
    gamma: int,
    max_runtime: float,
    max_iterations: float,
):
    """
    Tests that valid parameters are correctly set.
    """
    mat = MovingAverageThreshold(
        eta=eta,
        gamma=gamma,
        max_runtime=max_runtime,
        max_iterations=max_iterations,
    )

    assert_equal(mat.eta, eta)
    assert_equal(mat.gamma, gamma)
    assert_equal(mat.max_runtime, max_runtime)
    assert_equal(mat.max_iterations, max_iterations)


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
    mat = MovingAverageThreshold(eta=0.5, gamma=2)

    # First candidate is always accepted.
    assert_(mat(1, 1, 2))

    # Threshold is set at 2 + 0.5 * (3 - 2) = 2.5, so reject.
    assert_(not mat(1, 1, 4))

    # Threshold is set at 1 + 0.5 * (2.5 - 1) = 1.75, so accept.
    assert_(mat(1, 1, 1))

    # Threshold is set at 1 + 0.5 * (1.5 - 1) = 1.25, so reject.
    assert_(not mat(1, 1, 2))


def test_threshold_is_always_recent_best_with_gamma_one():
    """
    Tests that MAT always accepts the candidate solution if gamma is 1.
    """
    mat = MovingAverageThreshold(eta=0.5, gamma=1)

    # With gamma=1, the history always contains the candidate solution
    # and is of size 1, so the threshold is always equal to the candidate.
    assert_(mat(1, 1, 1))
    assert_(mat(1, 1, 10))
    assert_(mat(1, 1, 100))
    assert_(mat(1, 1, 1000))


def test_threshold_converges_with_max_runtime():
    """
    Tests that MAT correctly converges the threshold based on the runtime.
    """
    mat = MovingAverageThreshold(eta=1, gamma=10, max_runtime=0)

    mat(1, 1, 10)
    mat(1, 1, 1)

    # Even though the candidate solutions are (10, 1, 2), the factor is set
    # to 0 because we have reached the maximum runtime. MAT will then only
    # accept candidate solutions that are as good as the recent best solution.
    assert_(not mat(1, 1, 2))


def test_threshold_converges_with_max_iterations():
    """
    Tests that MAT correctly converges the threshold based on iterations.
    """
    mat = MovingAverageThreshold(eta=1, gamma=10, max_iterations=2)

    # Threshold is set at 2 + 1 * (2 - 2) = 2, so accept.
    assert_(mat(1, 1, 2))

    # Threshold is set at 1 + 0.5 * (1.5 - 1) = 1.25, so accept.
    assert_(mat(1, 1, 1))

    # We reached max iterations, so only candidates that are as good as recent
    # best solutions will be accepted.
    assert_(not mat(1, 1, 1.5))


def test_threshold_converges_with_most_restricting_limit():
    """
    Tests that MAT correctly converges the threshold based on the most
    restrictive condition.
    """
    mat = MovingAverageThreshold(
        eta=1, gamma=10, max_runtime=100, max_iterations=2
    )

    mat(1, 1, 2)
    mat(1, 1, 1)

    # Maximum iterations has been reached, so the factor is set to 0,
    # even though the maximum runtime hasn't elapsed yet.
    assert_(not mat(1, 1, 1.5))
