from typing import Protocol

from pyvrp._pyvrp import CostEvaluator, Solution


class SearchMethod(Protocol):  # pragma: no cover
    """
    Protocol that search methods must implement.
    """

    def __call__(
        self,
        solution: Solution,
        cost_evaluator: CostEvaluator,
        exhaustive: bool = False,
    ) -> Solution:
        """
        Search around the given solution, and returns a new solution that is
        hopefully better.

        Parameters
        ----------
        solution
            The solution to improve.
        cost_evaluator
            Cost evaluator to use when evaluating improvements.
        exhaustive
            Whether to perform an exhaustive search. When ``True``, the search
            method skips any randomizing perturbation steps and focuses purely
            on deterministic improvement moves. Default ``False``.

        Returns
        -------
        Solution
            The improved solution.
        """
