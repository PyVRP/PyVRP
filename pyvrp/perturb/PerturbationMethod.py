from typing import Protocol

from pyvrp._pyvrp import CostEvaluator, Solution


class PerturbationMethod(Protocol):  # pragma: no cover
    """
    Protocol that perturbation methods must implement.
    """

    def __call__(
        self,
        solution: Solution,
        cost_evaluator: CostEvaluator,
    ) -> Solution:
        """
        Perturb the given solution, and returns a new solution that is
        different from the passed-in solution.

        Parameters
        ----------
        solution
            The solution to perturb.
        cost_evaluator
            Cost evaluator to use when evaluating the perturbation.

        Returns
        -------
        Solution
            The perturbed solution.
        """
