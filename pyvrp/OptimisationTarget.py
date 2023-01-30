from numbers import Real
from typing import Protocol


class OptimisationTarget(Protocol):
    """
    Defines an optimisation target protocol.
    """

    def cost(self) -> Real:
        pass
