from typing import Protocol

from pyvrp.OptimisationTarget import OptimisationTarget


class StoppingCriterion(Protocol):  # pragma: no cover
    """
    Protocol that stopping criteria must implement.
    """

    def __call__(self, best: OptimisationTarget) -> bool:
        """
        When called, this stopping criterion should return True if the
        algorithm should stop, and False otherwise.

        Parameters
        ----------
        best
            Current best solution.

        Returns
        -------
        bool
            True if the algorithm should stop, False otherwise.
        """
        ...
