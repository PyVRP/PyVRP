class EmptySolutionWarning(UserWarning):
    """
    Raised when an empty solution is being added to the Population. This is not
    forbidden, per se, but very odd.
    """


class ScalingWarning(UserWarning):
    """
    Raised when the distance or duration values in the problem are very large,
    which could cause the algorithm to suffer from numerical issues.
    """


class TspWarning(UserWarning):
    """
    Raised when the problem is a TSP but a component is used that explicitly
    requires the presence of two or more vehicles (i.e., a proper VRP).
    """
