# This file is part of the PyVRP project (https://github.com/PyVRP/PyVRP), and
# licensed under the terms of the MIT license.


class ScalingWarning(UserWarning):
    """
    Raised when the distance or duration values in the problem exceed
    :const:`~pyvrp.constants.MAX_VALUE`, which could cause the algorithm to
    suffer from numerical issues.
    """


class PenaltyBoundWarning(UserWarning):
    """
    Raised when a penalty parameter has reached its maximum value. This means
    PyVRP struggles to find a feasible solution for the instance that's being
    solved, either because the instance has no feasible solution, or it is just
    very hard to find one.
    """
