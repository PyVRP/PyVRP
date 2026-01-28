from pyvrp._pyvrp import (
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Solution,
)
from pyvrp.search._search import (
    BinaryOperator,
    LocalSearchStatistics,
    PerturbationManager,
)
from pyvrp.search._search import LocalSearch as _LocalSearch


class LocalSearch:
    """
    Local search method. This search method explores a granular neighbourhood
    in a very efficient manner using user-provided operators. This quickly
    results in much improved solutions.

    Parameters
    ----------
    data
        Data object describing the problem to be solved.
    rng
        Random number generator.
    neighbours
        List of lists that defines the local search neighbourhood.
    perturbation_manager
        Perturbation manager that handles perturbation during each invocation.
    """

    def __init__(
        self,
        data: ProblemData,
        rng: RandomNumberGenerator,
        neighbours: list[list[int]],
        perturbation_manager: PerturbationManager = PerturbationManager(),
    ):
        self._ls = _LocalSearch(data, neighbours, perturbation_manager)
        self._rng = rng

    def add_node_operator(self, op: BinaryOperator):
        """
        Adds a node operator to this local search object. The node operator
        will be used to improve a solution.

        Parameters
        ----------
        op
            The node operator to add to this local search object.
        """
        self._ls.add_node_operator(op)

    @property
    def neighbours(self) -> list[list[int]]:
        """
        Returns the granular neighbourhood currently used by the local search.
        """
        return self._ls.neighbours

    @neighbours.setter
    def neighbours(self, neighbours: list[list[int]]):
        """
        Convenience method to replace the current granular neighbourhood used
        by the local search object.
        """
        self._ls.neighbours = neighbours

    @property
    def node_operators(self) -> list[BinaryOperator]:
        """
        Returns the node operators in use.
        """
        return self._ls.node_operators

    @property
    def statistics(self) -> LocalSearchStatistics:
        """
        Returns search statistics about the most recently improved solution.
        """
        return self._ls.statistics

    def __call__(
        self,
        solution: Solution,
        cost_evaluator: CostEvaluator,
        exhaustive: bool = False,
    ) -> Solution:
        """
        This method improves the given solution through a (default
        non-exhaustive) local search.

        Parameters
        ----------
        solution
            The solution to improve through local search.
        cost_evaluator
            Cost evaluator to use.
        exhaustive
            Performs an exhaustive, complete search if set. Otherwise does
            only a limited search over perturbed clients (default).

        Returns
        -------
        Solution
            The improved solution. This is not the same object as the
            solution that was passed in.
        """
        self._ls.shuffle(self._rng)
        return self._ls(solution, cost_evaluator, exhaustive)
