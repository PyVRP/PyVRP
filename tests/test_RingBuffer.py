# This file is part of the PyVRP project (https://github.com/PyVRP/PyVRP), and
# licensed under the terms of the MIT license.

from numpy.testing import assert_, assert_equal

from pyvrp.RingBuffer import RingBuffer


def test_ring_buffer():
    """
    Tests that the ring buffer correctly tracks recently inserted values, up to
    a fixed size, and can be cleared to reset its state.
    """
    buffer = RingBuffer(maxlen=2)
    assert_equal(buffer.maxlen, 2)
    assert_equal(len(buffer), 0)

    obj1 = object()
    obj2 = object()

    # Insert obj1, and test that the length and peek functions return the
    # correct values: 1 item, and peek at the next slot should return None
    # since we haven't set it yet.
    buffer.append(obj1)
    assert_equal(len(buffer), 1)
    assert_(buffer.peek() is None)

    # Append obj2, and check that the buffer now has two items. Peeking at
    # the next slot should wrap around, back to obj1, since we can store only
    # two items.
    buffer.append(obj2)
    assert_equal(len(buffer), 2)
    assert_(buffer.peek() is obj1)

    # Skip the next slot. This should not remove anything, but should cause
    # ``peek()`` to now point to the slot occupied by obj2.
    buffer.skip()
    assert_equal(len(buffer), 2)
    assert_(buffer.peek() is obj2)

    # Clearing the buffer should reset its entire state.
    buffer.clear()
    assert_equal(buffer.maxlen, 2)
    assert_equal(len(buffer), 0)
