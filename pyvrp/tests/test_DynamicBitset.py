from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp._DynamicBitset import DynamicBitset


@mark.parametrize(
    "num_bits, expected_size",
    [(0, 0), (1, 64), (64, 64), (65, 128), (128, 128)],
)
def test_size(num_bits, expected_size):
    """
    The bitset size is increased in increments of 64 bits.
    """
    bitset = DynamicBitset(num_bits)
    assert_equal(len(bitset), expected_size)


def test_init_all_zero():
    bitset = DynamicBitset(64)
    assert_(all(not bitset[idx] for idx in range(len(bitset))))
    assert_equal(bitset.count(), 0)


def test_get_set_item():
    bitset = DynamicBitset(128)
    indices = [0, 1, 63, 64, 126, 127]

    assert_equal(bitset.count(), 0)  # Bitset starts empty - no bits are set

    for idx in indices:  # flip all bits at the six indices to True
        bitset[idx] = True
        assert_(bitset[idx])

    assert_equal(bitset.count(), len(indices))  # now six bits should be set

    for idx in indices:  # flip all bits at the indices back to False
        bitset[idx] = False
        assert_(not bitset[idx])

    assert_equal(bitset.count(), 0)  # now no bits should be set


# TODO
