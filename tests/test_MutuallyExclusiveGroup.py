import pickle

from numpy.testing import assert_, assert_equal, assert_raises

from pyvrp import Activity, MutuallyExclusiveGroup


def test_mutually_exclusive_group_attribute_access():
    """
    Tests that the group's attributes are correctly set and accessible.
    """
    activities = [Activity("C1"), Activity("L1"), Activity("C3")]
    group = MutuallyExclusiveGroup(activities, required=False, name="test")

    assert_equal(group.activities, activities)
    assert_equal(group.required, False)
    assert_equal(group.name, "test")
    assert_equal(str(group), "test")


def test_mutually_exclusive_group_raises_duplicate_activities():
    """
    Tests that adding the same activity to a group more than once raises.
    """
    activity = Activity("C0")

    with assert_raises(ValueError):
        MutuallyExclusiveGroup([activity, activity])

    group = MutuallyExclusiveGroup()
    group.add_activity(activity)  # this should be OK

    with assert_raises(ValueError):
        group.add_activity(activity)  # but adding it a second time is not


def test_raises_invalid_activity_type():
    """
    Tests that groups accept only client and pickup activities.
    """
    with assert_raises(ValueError):
        MutuallyExclusiveGroup([Activity("D0")])

    group = MutuallyExclusiveGroup()
    with assert_raises(ValueError):  # depots are not OK
        group.add_activity(Activity("D0"))

    with assert_raises(ValueError):  # deliveries are also not OK
        group.add_activity(Activity("U0"))


def test_eq():
    """
    Tests the equality operator.
    """
    group1 = MutuallyExclusiveGroup(
        activities=[Activity("C1"), Activity("C2")], required=False
    )
    group2 = MutuallyExclusiveGroup(
        activities=[Activity("C1"), Activity("C2")], required=True
    )
    assert_(group1 != group2)

    # This group is equivalent to group1.
    group3 = MutuallyExclusiveGroup(
        activities=[Activity("C1"), Activity("C2")], required=False
    )
    assert_(group1 == group3)

    # And some things that are not mutually exclusive groups.
    assert_(group1 != "text")
    assert_(group1 != 7)

    group4 = MutuallyExclusiveGroup(
        activities=[Activity("C1"), Activity("C2")], required=False, name="t"
    )
    assert_(group1 != group4)

    # Activity types are part of the group identity, even for equal indices.
    group5 = MutuallyExclusiveGroup(
        activities=[Activity("C1"), Activity("L2")], required=False
    )
    assert_(group1 != group5)


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
    obj = MutuallyExclusiveGroup(
        activities=[Activity("C1"), Activity("L1"), Activity("C3")],
        required=False,
        name="t",
    )
    bytes = pickle.dumps(obj)
    assert_equal(pickle.loads(bytes), obj)
