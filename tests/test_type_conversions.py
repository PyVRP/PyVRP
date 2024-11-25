__doc__ = """
The tests in this file check that Python objects convertible to floats are
handled appropriately on the C++ side.
"""

import pytest
from numpy.testing import assert_equal, assert_raises

from pyvrp import Client


@pytest.mark.parametrize(
    ("input", "expected"),
    [
        (1, 1.0),
        (0, 0.0),
        (0.0, 0.0),
        (1.0, 1.0),
        (1.5, 1.5),
    ],
)
def test_measure_convertible_to_float(input, expected):
    """
    Tests that input argument for which float() succeeds are also properly
    handled on the C++ side.
    """
    client = Client(1, 1, tw_late=input)
    assert_equal(client.tw_late, expected)


def test_measure_raises_for_non_numerical_data():
    """
    Tests that input arguments that are not floats, or otherwise numeric data,
    raise a type error to warn the user.
    """
    with assert_raises(TypeError):
        Client(1, 1, tw_late="1")
