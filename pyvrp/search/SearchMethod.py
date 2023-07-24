from typing import Protocol

from pyvrp import Solution


class SearchMethod(Protocol):  # pragma: no cover
    """
    Protocol that search methods must implement.
    """

    def __call__(self, solution: Solution) -> Solution:
        """
        Search around the given solution, and returns a new solution that is
        hopefully better.

        Parameters
        ----------
        solution
            The solution to improve.

        Returns
        -------
        Solution
            The improved solution.
        """
        ...
