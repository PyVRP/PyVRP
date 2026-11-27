from pyvrp._pyvrp import (
    Activity,
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Solution,
)
from pyvrp.search._search import (
    BinaryOperator,
    LocalSearchStatistics,
    PerturbationManager,
    UnaryOperator,
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
        Mapping from each client or pickup activity to the activities in its
        granular neighbourhood.
    perturbation_manager
        Perturbation manager that handles perturbation during each invocation.
        Uses a default perturbation manager if not provided.
    """

    def __init__(
        self,
        data: ProblemData,
        rng: RandomNumberGenerator,
        neighbours: dict[Activity, list[Activity]],
        perturbation_manager: PerturbationManager | None = None,
    ):
        if perturbation_manager is None:
            perturbation_manager = PerturbationManager(data)

        self._ls = _LocalSearch(data, neighbours, perturbation_manager)
        self._rng = rng

    def add_operator(self, op: UnaryOperator | BinaryOperator):
        """
        Adds an operator to this local search object. The operator will be used
        to improve a solution.

        Parameters
        ----------
        op
            The operator to add to this local search object.
        """
        self._ls.add_operator(op)

    @property
    def neighbours(self) -> dict[Activity, list[Activity]]:
        """
        Returns the full neighbourhood used by perturbation and insertion.
        """
        return self._ls.neighbours

    @neighbours.setter
    def neighbours(self, neighbours: dict[Activity, list[Activity]]):
        """
        Convenience method to replace the current granular neighbourhood used
        by the local search object.
        """
        self._ls.neighbours = neighbours

    @property
    def active_neighbours(self) -> dict[Activity, list[Activity]]:
        """
        Returns the ordered neighbourhoods currently explored by local search.
        """
        return self._ls.active_neighbours

    def configure_dynamic_neighbourhood(
        self,
        candidates: dict[Activity, list[Activity]],
        patience: int = 1000,
    ):
        """
        Enables neighbourhoods that widen from quarter to half to full size.

        After ``patience`` unsuccessful participations, a client or shipment's
        selected prefix doubles. Finding a new best resets participating
        owners to quarter size. Reciprocal selection can activate additional
        entries within the given candidate lists.
        """
        self._ls.configure_dynamic_neighbourhood(candidates, patience)

    def update_neighbourhood(self, new_best: bool):
        """
        Updates the dynamic neighbourhood once per outer ILS iteration, after
        both ordinary and exhaustive local search have finished.
        """
        self._ls.update_neighbourhood(new_best)

    @property
    def unary_operators(self) -> list[UnaryOperator]:
        """
        Returns the unary operators in use.
        """
        return self._ls.unary_operators

    @property
    def binary_operators(self) -> list[BinaryOperator]:
        """
        Returns the binary operators in use.
        """
        return self._ls.binary_operators

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
