from numpy.testing import assert_, assert_equal, assert_raises
from pytest import mark

from pyvrp.accept import MovingBestAverageThreshold


@mark.parametrize(
    "initial_weight, history_length, max_runtime, max_iterations",
    [
        (-1, 1, 0, 0),  # initial_weight cannot be < 0
        (2, 1, 0, 0),  # initial_weight cannot be > 1
        (1, -2, 0, 0),  # history_length cannot be < 0
        (1, 0, 0, 0),  # history_length cannot be 0
        (1, 1, -1, 0),  # max_runtime cannot be < 0
        (1, 1, 0, -1),  # max_iterations cannot be < 0
    ],
)
def test_raise_invalid_parameters(
    initial_weight: float,
    history_length: int,
    max_runtime: float,
    max_iterations: int,
):
    """
    Tests that a ValueError is raised when invalid parameters are passed.
    """
    with assert_raises(ValueError):
        MovingBestAverageThreshold(
            initial_weight=initial_weight,
            history_length=history_length,
            max_runtime=max_runtime,
            max_iterations=max_iterations,
        )


@mark.parametrize(
    "initial_weight, history_length, max_runtime, max_iterations",
    [
        (1, 2, 3, 4),
        (0.4, 4, 10.1, 100),
    ],
)
def test_attributes_are_correctly_set(
    initial_weight: float,
    history_length: int,
    max_runtime: float,
    max_iterations: int,
):
    """
    Tests that valid parameters are correctly set.
    """
    mbat = MovingBestAverageThreshold(
        initial_weight=initial_weight,
        history_length=history_length,
        max_runtime=max_runtime,
        max_iterations=max_iterations,
    )

    assert_equal(mbat.initial_weight, initial_weight)
    assert_equal(mbat.history_length, history_length)
    assert_equal(mbat.max_runtime, max_runtime)
    assert_equal(mbat.max_iterations, max_iterations)


def test_accepts_below_threshold():
    """
    Tests that MBAT accepts a candidate solution below the threshold.
    """
    mbat = MovingBestAverageThreshold(initial_weight=0.5, history_length=4)
    mbat(1, 1, 1)
    mbat(1, 1, 2)

    # The threshold is set at 0 + 0.5 * (1 - 0) = 0.5, candidate has cost 0.
    assert_(mbat(1, 1, 0))


def test_rejects_above_threshold():
    """
    Tests that MBAT rejects a candidate solution above the threshold.
    """
    mbat = MovingBestAverageThreshold(initial_weight=0.5, history_length=4)
    mbat(1, 1, 2)
    mbat(1, 1, 0)

    # The threshold is set at 0 + 0.5 * (1 - 0) = 0.5, candidate has cost 1.
    assert_(not mbat(1, 1, 1))


def test_accepts_equal_threshold():
    """
    Tests that MBAT accepts a candidate solution equal to the threshold.
    """
    mbat = MovingBestAverageThreshold(initial_weight=0.5, history_length=4)
    mbat(1, 1, 1)
    mbat(1, 1, 1)

    # The threshold is set at 1 + 0.5 * (1 - 1) = 1, candidate has cost 1.
    assert_(mbat(1, 1, 1))


def test_threshold_updates_with_history():
    """
    Tests that MBAT correctly updates its history by replacing older solutions.
    """
    mbat = MovingBestAverageThreshold(initial_weight=0.5, history_length=10)

    # Populate the history with solutions of cost 1.
    for _ in range(10):
        mbat(1, 1, 1)

    # The next nine solutions with cost 2 are rejected...
    for _ in range(9):
        assert_(not mbat(1, 1, 2))

    # ...but the 10th solution with cost 2 is accepted because now the
    # history only consists of solutions with cost 2.
    assert_(mbat(1, 1, 2))


def test_always_accept_candidate_with_history_length_one():
    """
    Tests that MBAT always accepts the candidate if history length is 1.
    """
    mbat = MovingBestAverageThreshold(initial_weight=0.5, history_length=1)

    # With a history length of one, the history is just the candidate solution,
    # so the threshold is always equal to the candidate.
    assert_(mbat(1, 1, 1))
    assert_(mbat(1, 1, 10))
    assert_(mbat(1, 1, 100))
    assert_(mbat(1, 1, 1000))


def test_only_accept_better_than_recent_best_with_weight_zero():
    """
    Tests that MBAT only accepts candidates that are as good as the recent best
    solution when the weight is zero.
    """
    mbat = MovingBestAverageThreshold(initial_weight=0, history_length=100)

    assert_(mbat(1, 1, 1))

    assert_(not mbat(1, 1, 1000))
    assert_(not mbat(1, 1, 1000))

    # The weight is zero, so despite the recent average being very high, this
    # candidate is still rejected because it is worse than the recent best.
    assert_(not mbat(1, 1, 1.01))
    assert_(mbat(1, 1, 1))


def test_threshold_converges_with_zero_max_runtime():
    """
    Tests that MBAT correctly converges the threshold based on the runtime.
    """
    mbat = MovingBestAverageThreshold(
        initial_weight=1, history_length=10, max_runtime=0
    )

    # First solution is always accepted.
    assert_(mbat(1, 1, 1))

    # The dynamic weight is set to 0 because we have reached the maximum
    # runtime. MBAT now only accepts candidate solutions that are as good as
    # the recent best solution.
    assert_(not mbat(1, 1, 5))
    assert_(not mbat(1, 1, 1.01))
    assert_(mbat(1, 1, 1))


def test_threshold_converges_with_zero_max_iterations():
    """
    Tests that MBAT correctly converges the threshold based on iterations.
    """
    mbat = MovingBestAverageThreshold(
        initial_weight=1, history_length=10, max_iterations=0
    )

    # First solution is always accepted.
    assert_(mbat(1, 1, 1))

    # The dynamic weight is set to 0 because we have reached the maximum
    # runtime. MBAT now only accepts candidate solutions that are as good as
    # the recent best solution.
    assert_(not mbat(1, 1, 3))
    assert_(not mbat(1, 1, 2))


def test_threshold_converges_with_max_iterations():
    """
    Tests that MBAT correctly converges the threshold based on iterations.
    """
    mbat = MovingBestAverageThreshold(
        initial_weight=1, history_length=10, max_iterations=2
    )

    # First solution is always accepted.
    assert_(mbat(1, 1, 2))

    # Threshold is set at 1 + 0.5 * (1.5 - 1) = 1.25, so accept.
    assert_(mbat(1, 1, 1))

    # We reached max iterations, so only candidates that are as good as recent
    # best solutions will be accepted.
    assert_(not mbat(1, 1, 1.01))


def test_threshold_converges_with_most_restricting_limit():
    """
    Tests that MBAT correctly converges the threshold based on the most
    restrictive condition.
    """
    mbat = MovingBestAverageThreshold(
        initial_weight=1, history_length=10, max_runtime=100, max_iterations=2
    )

    mbat(1, 1, 10)
    mbat(1, 1, 1)

    # Maximum iterations has been reached, so the weight is set to 0,
    # even though the maximum runtime hasn't elapsed yet. MBAT now only
    # accepts candidates that are as good as the recent best solution.
    assert_(not mbat(1, 1, 2))
