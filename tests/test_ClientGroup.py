import pickle

from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import ClientGroup


def test_client_group_attribute_access():
    """
    Tests that the ClientGroup's attributes are correctly set and accessible.
    """
    group = ClientGroup(clients=[1, 2, 3], required=False, name="test")

    assert_equal(group.clients, [1, 2, 3])
    assert_equal(group.required, False)
    assert_equal(group.mutually_exclusive, True)
    assert_equal(group.name, "test")
    assert_equal(str(group), "test")


def test_client_group_raises_duplicate_clients():
    """
    Tests that adding the same client to a group more than once raises.
    """
    with assert_raises(ValueError):
        ClientGroup([1, 1])

    group = ClientGroup()
    group.add_client(1)  # this should be OK

    with assert_raises(ValueError):
        group.add_client(1)  # but adding the client a second time is not


def test_eq():
    """
    Tests the equality operator.
    """
    group1 = ClientGroup(clients=[1, 2], required=False)
    group2 = ClientGroup(clients=[1, 2], required=True)
    assert_(group1 != group2)

    # This group is equivalent to group1.
    group3 = ClientGroup(clients=[1, 2], required=False)
    assert_(group1 == group3)

    # And some things that are not client groups.
    assert_(group1 != "text")
    assert_(group1 != 7)

    group4 = ClientGroup(clients=[1, 2], required=False, name="test")
    assert_(group1 != group4)


def test_eq_name():
    """
    Tests that the equality operator considers names.
    """
    assert_(ClientGroup(name="1") != ClientGroup(name="2"))


def test_pickle():
    """
    Tests that client groups can be serialised and unserialised.
    """
    before_pickle = ClientGroup(clients=[1, 2, 3], required=False, name="test")
    bytes = pickle.dumps(before_pickle)
    assert_equal(pickle.loads(bytes), before_pickle)
