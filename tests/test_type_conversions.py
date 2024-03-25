__doc__ = """
The tests in this file check that Python objects that can be converted to
integers are handled appropriately on the C++ side, including potential
overflows.
"""

import numpy as np
import pytest
from numpy.testing import assert_equal, assert_raises

from pyvrp import Depot


@pytest.mark.parametrize(
    ("input", "expected"),
    [
        (1, 1),
        (0, 0),
        ("1", 1),
        (1.0, 1),
        (1.5, 1),
        (2**31 - 1, 2**31 - 1),
        (2**31, 2**31),
        (2**63 - 1, 2**63 - 1),  # largest supported 64-bit signed value
    ],
)
def test_measure_convertible_to_int(input, expected):
    """
    Tests that input argument for which int() succeeds are also properly
    handled on the C++ side.
    """
    depot = Depot(1, 1, tw_late=input)
    assert_equal(depot.tw_late, expected)


def test_larger_than_max_size():
    """
    Tests that arguments larger than the maximum value for measure arguments
    raise an overflow error to warn the user.
    """
    max_size = np.iinfo(np.int64).max

    with assert_raises(OverflowError):
        Depot(1, 1, tw_late=max_size + 1)
