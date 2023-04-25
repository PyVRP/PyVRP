from numpy.testing import assert_equal
from pytest import mark

from pyvrp._Measure import (
    cost_type,
    distance_type,
    duration_type,
    has_integer_precision,
)


@mark.parametrize("num", [1, 2.0, 3.0, 1000.067])
def test_type_casting(num):
    cast = int if has_integer_precision else float

    assert_equal(float(distance_type(num)), cast(num))
    assert_equal(int(distance_type(num)), cast(num))

    assert_equal(float(duration_type(num)), cast(num))
    assert_equal(int(duration_type(num)), cast(num))

    assert_equal(float(cost_type(num)), cast(num))
    assert_equal(int(cost_type(num)), cast(num))
