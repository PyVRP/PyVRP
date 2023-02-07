from typing import Protocol, Union


class OptimisationTarget(Protocol):  # pragma: no cover
    """
    Defines an optimisation target protocol.
    """

    def cost(self) -> Union[int, float]:
        pass
