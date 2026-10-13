from typing import Protocol


class StoppingCriterion(Protocol):  # pragma: no cover
    """
    Protocol that stopping criteria must implement.
    """

    def __call__(self, best_cost: float) -> bool:
        """
        When called, this stopping criterion should return True if the
        algorithm should stop, and False otherwise.

        Parameters
        ----------
        best_cost
            Cost of current best solution.

        Returns
        -------
        bool
            True if the algorithm should stop, False otherwise.
        """
