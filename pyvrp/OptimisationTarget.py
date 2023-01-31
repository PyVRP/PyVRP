from numbers import Real
from typing import Protocol


class OptimisationTarget(Protocol):  # pragma: no cover
    """
    Defines an optimisation target protocol.
    """

    def cost(self) -> Real:
        pass
