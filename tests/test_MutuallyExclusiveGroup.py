import pickle

from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import MutuallyExclusiveGroup


def test_mutually_exclusive_group_attribute_access():
    """
    Tests that the group's attributes are correctly set and accessible.
    """
    group = MutuallyExclusiveGroup(
        clients=[1, 2, 3], required=False, name="test"
    )

    assert_equal(group.clients, [1, 2, 3])
    assert_equal(group.required, False)
    assert_equal(group.name, "test")
    assert_equal(str(group), "test")


def test_mutually_exclusive_group_raises_duplicate_clients():
    """
    Tests that adding the same client to a group more than once raises.
    """
    with assert_raises(ValueError):
        MutuallyExclusiveGroup([1, 1])

    group = MutuallyExclusiveGroup()
    group.add_client(1)  # this should be OK

    with assert_raises(ValueError):
        group.add_client(1)  # but adding the client a second time is not


def test_eq():
    """
    Tests the equality operator.
    """
    group1 = MutuallyExclusiveGroup(clients=[1, 2], required=False)
    group2 = MutuallyExclusiveGroup(clients=[1, 2], required=True)
    assert_(group1 != group2)

    # This group is equivalent to group1.
    group3 = MutuallyExclusiveGroup(clients=[1, 2], required=False)
    assert_(group1 == group3)

    # And some things that are not mutually exclusive groups.
    assert_(group1 != "text")
    assert_(group1 != 7)

    group4 = MutuallyExclusiveGroup(clients=[1, 2], required=False, name="t")
    assert_(group1 != group4)


def test_eq_name():
    """
    Tests that the equality operator considers names.
    """
    group1 = MutuallyExclusiveGroup(name="1")
    group2 = MutuallyExclusiveGroup(name="2")
    assert_(group1 != group2)


def test_pickle():
    """
    Tests that mutually exclusive groups can be serialised and unserialised.
    """
    obj = MutuallyExclusiveGroup(clients=[1, 2, 3], required=False, name="t")
    bytes = pickle.dumps(obj)
    assert_equal(pickle.loads(bytes), obj)
