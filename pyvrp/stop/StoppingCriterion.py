from typing import Protocol


class StoppingCriterion(Protocol):  # pragma: no cover
    """
    Protocol that stopping criteria must implement.
    """

    def fraction_remaining(self) -> float | None:
        """
        Returns the fraction of the stopping criterion's budget that remains.

        Returns
        -------
        float | None
            A value between 0 and 1, where 0 means the stopping criterion is
            met and 1 means it is not. If the stopping criterion does not
            have a meaningful interpretation of remaining fraction, it should
            return None.
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
