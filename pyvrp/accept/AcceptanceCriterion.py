from typing import Protocol


class AcceptanceCriterion(Protocol):
    """
    Protocol describing an acceptance criterion.
    """

    def __call__(self, best: float, current: float, candidate: float) -> bool:
        """
        Determines whether to accept the proposed, candidate solution based on
        this acceptance criterion and the solution costs.

        Parameters
        ----------
        best
            The cost of the best solution observed so far.
        current
            The cost of the current solution.
        candidate
            The cost of the proposed, candidate solution.

        Returns
        -------
        bool
            Whether to accept the candidate solution (True), or not (False).
        """
        ...
