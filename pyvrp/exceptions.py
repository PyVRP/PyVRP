class EmptySolutionWarning(UserWarning):
    """
    Raised when an empty solution is being added to the Population. This is not
    forbidden, per se, but very odd.
    """


class ScalingWarning(UserWarning):
    """
    Raised when the distance or duration values in the problem are very large,
    which could cause the algorithm to start using forbidden edges as well.
    """
