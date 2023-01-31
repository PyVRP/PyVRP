from typing import Protocol

from pyvrp.OptimisationTarget import OptimisationTarget


class StoppingCriterion(Protocol):  # pragma: no cover
    """
    Protocol that stopping criteria must implement.
    """

    def __call__(self, best: OptimisationTarget) -> bool:
        pass
