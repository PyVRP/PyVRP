from numpy.testing import assert_, assert_equal
from pytest import mark

from pyvrp._pyvrp import DynamicBitset


@mark.parametrize(
    ("num_bits", "expected_size"),
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


def test_eq():
    bitset1 = DynamicBitset(64)
    bitset2 = DynamicBitset(64)
    assert_(bitset1 == bitset2)  # both empty, equal size - should be the same

    bitset2[0] = True
    assert_(bitset1 != bitset2)  # 2 is no longer empty; should not be the same

    # Test against a few things that are not bitsets, to make sure __eq__ can
    # handle that as well.
    assert_(bitset1 != 0)
    assert_(bitset1 != "test")


def test_get_set_item():
    bitset = DynamicBitset(128)
    indices = [0, 1, 63, 64, 126, 127]

    assert_equal(bitset.count(), 0)  # Bitset starts empty - no bits are set

    for idx in indices:  # flip all bits at the six indices to True
        assert_(not bitset[idx])
        bitset[idx] = True
        assert_(bitset[idx])

    assert_equal(bitset.count(), len(indices))  # now six bits should be set

    for idx in indices:  # flip all bits at the indices back to False
        assert_(bitset[idx])
        bitset[idx] = False
        assert_(not bitset[idx])

    assert_equal(bitset.count(), 0)  # now no bits should be set


def test_bit_or():
    bitset1 = DynamicBitset(128)
    bitset1[0] = True
    bitset1[64] = True

    bitset2 = DynamicBitset(128)
    bitset2[0] = True
    bitset2[65] = True

    bitset3 = bitset1 | bitset2
    assert_equal(bitset3.count(), 3)
    assert_(bitset3[0])
    assert_(bitset3[64])
    assert_(bitset3[65])


def test_bit_and():
    bitset1 = DynamicBitset(128)
    bitset1[0] = True
    bitset1[64] = True

    bitset2 = DynamicBitset(128)
    bitset2[0] = True
    bitset2[65] = True

    bitset3 = bitset1 & bitset2
    assert_equal(bitset3.count(), 1)
    assert_(bitset3[0])
    assert_(not bitset3[64])
    assert_(not bitset3[65])


def test_bit_xor():
    bitset1 = DynamicBitset(128)
    bitset1[0] = True
    bitset1[64] = True

    bitset2 = DynamicBitset(128)
    bitset2[0] = True
    bitset2[65] = True

    bitset3 = bitset1 ^ bitset2
    assert_equal(bitset3.count(), 2)
    assert_(not bitset3[0])
    assert_(bitset3[64])
    assert_(bitset3[65])


def test_bit_not():
    bitset = DynamicBitset(128)
    assert_equal(bitset.count(), 0)

    inverted = ~bitset
    assert_equal(inverted.count(), 128)

    inverted[0] = False
    inverted[127] = False
    assert_equal((~inverted).count(), 2)
