from numpy.testing import assert_, assert_raises
from pytest import mark

from pyvrp.stop import TimedNoImprovement
from pyvrp.tests.helpers import sleep


@mark.parametrize(
    ("max_iterations", "max_runtime"),
    [(-1, 0), (-42, 0), (-10_000, 0), (0, -1), (0, -42), (0, -10_000)],
)
def test_raise_negative_parameters(max_iterations, max_runtime):
    with assert_raises(ValueError):
        TimedNoImprovement(max_iterations, max_runtime)


@mark.parametrize(
    ("max_iterations", "max_runtime"), [(0, 0.001), (1, 0.01), (10, 0.0)]
)
def test_valid_parameters(max_iterations, max_runtime):
    """
    Does not raise for non-negative parameters.
    """
    TimedNoImprovement(max_iterations, max_runtime)


def test_stops_if_zero_max_iterations():
    stop = TimedNoImprovement(0, 0.100)
    assert_(stop(1))


@mark.parametrize("max_runtime", [0.01, 0.05, 0.10])
def test_before_max_runtime(max_runtime):
    stop = TimedNoImprovement(101, max_runtime)

    for _ in range(100):
        assert_(not stop(1))


@mark.parametrize("max_runtime", [0.01, 0.05, 0.10])
def test_after_max_runtime(max_runtime):
    stop = TimedNoImprovement(101, max_runtime)
    assert_(not stop(1))  # trigger the first time measurement

    sleep(max_runtime)

    for _ in range(100):
        assert_(stop(1))
