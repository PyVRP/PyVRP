from typing import Protocol

from pyvrp._pyvrp import CostEvaluator, Solution


class SearchMethod(Protocol):  # pragma: no cover
    """
    Protocol that search methods must implement.

    Search methods may additionally provide ``update_neighbourhood(new_best)``.
    ILS calls this optional hook once per iteration, after all local searches
    and before the iteration callback, to adapt the search neighbourhood.
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
            Whether to explicitly require a complete search, rather than allow
            the search method to perform a limited search. Default ``False``,
            that is, the search method gets to decide for itself what to do.

        Returns
        -------
        Solution
            The improved solution.
        """
