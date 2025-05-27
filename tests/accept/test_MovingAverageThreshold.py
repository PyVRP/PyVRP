from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.accept import MovingAverageThreshold


@mark.parametrize(
    "weight, history_length, max_runtime, max_iterations",
    [
        (-1, 1, 0, 0),  # weight cannot be < 0
        (1, -2, 0, 0),  # history_length cannot be < 0
        (1, 0, 0, 0),  # history_length cannot be 0
        (1, 1, -1, 0),  # max_runtime cannot be < 0
        (1, 1, 0, -1),  # max_iterations cannot be < 0
    ],
)
def test_raise_invalid_parameters(
    weight: float, history_length: int, max_runtime: float, max_iterations: int
):
    """
    Tests that a ValueError is raised when invalid parameters are passed.
    """
    with assert_raises(ValueError):
        MovingAverageThreshold(
            weight=weight,
            history_length=history_length,
            max_runtime=max_runtime,
            max_iterations=max_iterations,
        )


@mark.parametrize(
    "weight, history_length, max_runtime, max_iterations",
    [
        (1, 2, 3, 4),
        (0.4, 4, 10.1, 100),
    ],
)
def test_attributes_are_correctly_set(
    weight: float,
    history_length: int,
    max_runtime: float,
    max_iterations: int,
):
    """
    Tests that valid parameters are correctly set.
    """
    mat = MovingAverageThreshold(
        weight=weight,
        history_length=history_length,
        max_runtime=max_runtime,
        max_iterations=max_iterations,
    )

    assert_equal(mat.weight, weight)
    assert_equal(mat.history_length, history_length)
    assert_equal(mat.max_runtime, max_runtime)
    assert_equal(mat.max_iterations, max_iterations)


def test_accepts_below_threshold():
    """
    Tests that MAT accepts a candidate solution below the threshold.
    """
    mat = MovingAverageThreshold(weight=0.5, history_length=4)
    mat(1, 1, 1)
    mat(1, 1, 2)

    # The threshold is set at 0 + 0.5 * (1 - 0) = 0.5, candidate has cost 0.
    assert_(mat(1, 1, 0))


def test_rejects_above_threshold():
    """
    Tests that MAT rejects a candidate solution above the threshold.
    """
    mat = MovingAverageThreshold(weight=0.5, history_length=4)
    mat(1, 1, 2)
    mat(1, 1, 0)

    # The threshold is set at 0 + 0.5 * (1 - 0) = 0.5, candidate has cost 1.
    assert_(not mat(1, 1, 1))


def test_accepts_equal_threshold():
    """
    Tests that MAT accepts a candidate solution equal to the threshold.
    """
    mat = MovingAverageThreshold(weight=0.5, history_length=4)
    mat(1, 1, 1)
    mat(1, 1, 1)

    # The threshold is set at 1 + 0.5 * (1 - 1) = 1, candidate has cost 1.
    assert_(mat(1, 1, 1))


def test_threshold_updates_with_history():
    """
    Tests that MAT correctly updates the threshold based on the history of
    candidate solutions and also forgets older solutions.
    """
    mat = MovingAverageThreshold(weight=0.5, history_length=2)

    # First candidate is always accepted.
    assert_(mat(1, 1, 2))

    # Threshold is set at 2 + 0.5 * (3 - 2) = 2.5, so reject.
    assert_(not mat(1, 1, 4))

    # Threshold is set at 1 + 0.5 * (2.5 - 1) = 1.75, so accept.
    assert_(mat(1, 1, 1))

    # Threshold is set at 1 + 0.5 * (1.5 - 1) = 1.25, so reject.
    assert_(not mat(1, 1, 2))


def test_always_accept_candidate_with_history_length_one():
    """
    Tests that MAT always accepts the candidate if history length is 1.
    """
    mat = MovingAverageThreshold(weight=0.5, history_length=1)

    # With history_length=1, the history always contains the candidate solution
    # and is of size 1, so the threshold is always equal to the candidate.
    assert_(mat(1, 1, 1))
    assert_(mat(1, 1, 10))
    assert_(mat(1, 1, 100))
    assert_(mat(1, 1, 1000))


def test_only_accept_better_than_recent_best_with_weight_zero():
    """
    Tests that MAT only accepts candidates that are as good as the recent best
    solution when the weight is zero.
    """
    mat = MovingAverageThreshold(weight=0, history_length=100)

    assert_(mat(1, 1, 1))

    assert_(not mat(1, 1, 1000))
    assert_(not mat(1, 1, 1000))

    # The weight is zero, so despite the recent average being very high, this
    # candidate is still rejected because it is worse than the recent best.
    assert_(not mat(1, 1, 1.01))


def test_threshold_converges_with_zero_max_runtime():
    """
    Tests that MAT correctly converges the threshold based on the runtime.
    """
    mat = MovingAverageThreshold(weight=1, history_length=10, max_runtime=0)

    # First solution is always accepted.
    assert_(mat(1, 1, 1))

    # The dynamic weight is set to 0 because we have reached the maximum
    # runtime. MAT now only accepts candidate solutions that are as good as the
    # recent best solution.
    assert_(not mat(1, 1, 5))

    # This candidate is worse than recent best (1), so reject.
    assert_(not mat(1, 1, 2))


def test_threshold_converges_with_zero_max_iterations():
    """
    Tests that MAT correctly converges the threshold based on iterations.
    """
    mat = MovingAverageThreshold(weight=1, history_length=10, max_iterations=0)

    # First solution is always accepted.
    assert_(mat(1, 1, 1))

    # The dynamic weight is set to 0 because we have reached the maximum
    # runtime. MAT now only accepts candidate solutions that are as good as the
    # recent best solution.
    assert_(not mat(1, 1, 3))

    # This candidate is worse than recent best (1), so reject.
    assert_(not mat(1, 1, 2))


def test_threshold_converges_with_max_iterations():
    """
    Tests that MAT correctly converges the threshold based on iterations.
    """
    mat = MovingAverageThreshold(weight=1, history_length=10, max_iterations=2)

    # First solution is always accepted.
    assert_(mat(1, 1, 2))

    # Threshold is set at 1 + 0.5 * (1.5 - 1) = 1.25, so accept.
    assert_(mat(1, 1, 1))

    # We reached max iterations, so only candidates that are as good as recent
    # best solutions will be accepted.
    assert_(not mat(1, 1, 1.01))


def test_threshold_converges_with_most_restricting_limit():
    """
    Tests that MAT correctly converges the threshold based on the most
    restrictive condition.
    """
    mat = MovingAverageThreshold(
        weight=1, history_length=10, max_runtime=100, max_iterations=2
    )

    mat(1, 1, 10)
    mat(1, 1, 1)

    # Maximum iterations has been reached, so the weight is set to 0,
    # even though the maximum runtime hasn't elapsed yet. MAT now only
    # accepts candidates that are as good as the recent best solution.
    assert_(not mat(1, 1, 2))
