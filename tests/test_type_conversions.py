__doc__ = """
The tests in this file check that Python objects convertible to floats are
handled appropriately on the C++ side - this includes checking for potential
overflows and meaningful responses in that case.
"""

import numpy as np
import pytest
from numpy.testing import assert_allclose, assert_raises

from pyvrp import Client


@pytest.mark.parametrize(
    ("input", "expected"),
    [
        (1, 1),
        (0, 0),
        ("1", 1),
        (1.0, 1),
        (1.5, 1.5),
        (2**31 - 1, 2**31 - 1),
        (2**31, 2**31),
        (2**63 - 1, float(2**63 - 1)),
        (2**63, float(2**63)),
        (np.finfo(np.float64).max, np.finfo(np.float64).max),
        (np.float32(1.5), 1.5),
        (np.float64(1.5), 1.5),
        ("1.5", 1.5),
    ],
)
def test_measure_convertible_to_float(input, expected):
    """
    Tests that input argument for which float() succeeds are also properly
    handled on the C++ side.
    """
    client = Client(0, tw_late=input)
    assert_allclose(client.tw_late, expected)
    assert isinstance(client.tw_late, float)


@pytest.mark.parametrize("input", [10**309, 10**1000])
def test_larger_than_max_size(input):
    """
    Tests that arguments larger than the maximum value for measure arguments
    raise an overflow error to warn the user.
    """
    with assert_raises(OverflowError):
        Client(0, tw_late=input)


@pytest.mark.parametrize(
    ("value", "error"),
    [(None, TypeError), (object(), TypeError), ("x", ValueError)],
)
def test_invalid_measure_conversion(value, error):
    with assert_raises(error):
        Client(0, tw_late=value)
