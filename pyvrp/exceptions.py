class StatisticsNotCollectedError(Exception):
    """
    Raised when statistics have not been collected, but a method is accessed
    that can only be used when statistics are available.
    """

    pass


class EmptySolutionWarning(UserWarning):
    """
    Raised when an empty solution is being added to the Population. This is not
    forbidden, per se, but very odd.
    """

    pass
