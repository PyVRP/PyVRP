from pyvrp._pyvrp import (
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Solution,
)
from pyvrp.search._search import LocalSearch as _LocalSearch
from pyvrp.search._search import (
    LocalSearchStatistics,
    NodeOperator,
    PerturbationOperator,
    RouteOperator,
)


class LocalSearch:
    """
    Local search method. This search method explores a granular neighbourhood
    in a very efficient manner using user-provided node and route operators.
    This quickly results in much improved solutions.

    Parameters
    ----------
    data
        Data object describing the problem to be solved.
    rng
        Random number generator.
    neighbours
        List of lists that defines the local search neighbourhood.
    num_perturbations
        Maximum number of perturbations to apply in each iteration. The actual
        number of perturbations used is randomly chosen between 1 and this
        value (inclusive). Default is 25.
    """

    def __init__(
        self,
        data: ProblemData,
        rng: RandomNumberGenerator,
        neighbours: list[list[int]],
        num_perturbations: int = 25,
    ):
        self._ls = _LocalSearch(data, neighbours)
        self._rng = rng
        self._num_perturbations = num_perturbations

    def add_node_operator(self, op: NodeOperator):
        """
        Adds a node operator to this local search object. The node operator
        will be used by :meth:`~search` to improve a solution.

        Parameters
        ----------
        op
            The node operator to add to this local search object.
        """
        self._ls.add_node_operator(op)

    def add_route_operator(self, op: RouteOperator):
        """
        Adds a route operator to this local search object. The route operator
        will be used by :meth:`~intensify` to improve a solution using more
        expensive route operators.

        Parameters
        ----------
        op
            The route operator to add to this local search object.
        """
        self._ls.add_route_operator(op)

    def add_perturbation_operator(self, op: PerturbationOperator):
        """
        Adds a perturbation operator to this local search object. The
        perturbation operator will be used by :meth:`~perturb` to modify
        a solution to potentially escape local optima.

        Parameters
        ----------
        op
            The perturbation operator to add to this local search object.
        """
        self._ls.add_perturbation_operator(op)

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
    def node_operators(self) -> list[NodeOperator]:
        """
        Returns the node operators in use.
        """
        return self._ls.node_operators

    @property
    def route_operators(self) -> list[RouteOperator]:
        """
        Returns the route operators in use.
        """
        return self._ls.route_operators

    @property
    def perturbation_operators(self) -> list[PerturbationOperator]:
        """
        Returns the perturbation operators in use.
        """
        return self._ls.perturbation_operators

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
    ) -> Solution:
        """
        This method uses the :meth:`~search` and :meth:`~intensify` methods to
        iteratively improve the given solution. First, :meth:`~search` is
        applied. Thereafter, :meth:`~intensify` is applied. This repeats until
        no further improvements are found. Finally, the improved solution is
        returned.

        Parameters
        ----------
        solution
            The solution to improve through local search.
        cost_evaluator
            Cost evaluator to use.

        Returns
        -------
        Solution
            The improved solution. This is not the same object as the
            solution that was passed in.
        """
        num_perturbations = self._rng.randint(self._num_perturbations) + 1
        self._ls.num_perturbations = num_perturbations

        self._ls.shuffle(self._rng)
        return self._ls(solution, cost_evaluator)

    def intensify(
        self,
        solution: Solution,
        cost_evaluator: CostEvaluator,
    ) -> Solution:
        """
        This method uses the intensifying route operators on this local search
        object to improve the given solution.

        Parameters
        ----------
        solution
            The solution to improve.
        cost_evaluator
            Cost evaluator to use.

        Returns
        -------
        Solution
            The improved solution. This is not the same object as the
            solution that was passed in.
        """
        self._ls.shuffle(self._rng)
        return self._ls.intensify(solution, cost_evaluator)

    def search(
        self, solution: Solution, cost_evaluator: CostEvaluator
    ) -> Solution:
        """
        This method uses the node operators on this local search object to
        improve the given solution.

        Parameters
        ----------
        solution
            The solution to improve.
        cost_evaluator
            Cost evaluator to use.

        Returns
        -------
        Solution
            The improved solution. This is not the same object as the
            solution that was passed in.
        """
        self._ls.shuffle(self._rng)
        return self._ls.search(solution, cost_evaluator)

    def perturb(
        self,
        solution: Solution,
        cost_evaluator: CostEvaluator,
    ) -> Solution:
        """
        This method uses the perturbation operators on this local search object
        to modify the given solution.

        Parameters
        ----------
        solution
            The solution to perturb.
        cost_evaluator
            Cost evaluator to use.

        Returns
        -------
        Solution
            The perturbed solution. This is not the same object as the
            solution that was passed in.
        """
        num_perturbations = self._rng.randint(self._num_perturbations) + 1
        self._ls.num_perturbations = num_perturbations

        self._ls.shuffle(self._rng)
        return self._ls.perturb(solution, cost_evaluator)
