from __future__ import annotations

from typing import TYPE_CHECKING, Callable, Generator, Tuple
from warnings import warn

from ._pyvrp import PopulationParams, SubPopulation
from .exceptions import EmptySolutionWarning

if TYPE_CHECKING:
    from ._pyvrp import CostEvaluator, RandomNumberGenerator, Solution


class Population:
    """
    Creates a Population instance.

    Parameters
    ----------
    diversity_op
        Operator to use to determine pairwise diversity between solutions. Have
        a look at :mod:`pyvrp.diversity` for available operators.
    params, optional
        Population parameters. If not provided, a default will be used.
    """

    def __init__(
        self,
        diversity_op: Callable[[Solution, Solution], float],
        params: PopulationParams = PopulationParams(),
    ):
        self._op = diversity_op
        self._params = params

        self._feas = SubPopulation(diversity_op, params)
        self._infeas = SubPopulation(diversity_op, params)

    def __iter__(self) -> Generator[Solution, None, None]:
        """
        Iterates over the solutions contained in this population.

        Yields
        ------
        iterable
            An iterable object of solutions.
        """
        for item in self._feas:
            yield item.solution

        for item in self._infeas:
            yield item.solution

    def __len__(self) -> int:
        """
        Returns the current population size.

        Returns
        -------
        int
            Population size.
        """
        return len(self._feas) + len(self._infeas)

    def _update_fitness(self, cost_evaluator: CostEvaluator):
        """
        Updates the biased fitness values for the subpopulations.

        Parameters
        ----------
        cost_evaluator
            CostEvaluator to use for computing the fitness.
        """
        self._feas.update_fitness(cost_evaluator)
        self._infeas.update_fitness(cost_evaluator)

    def num_feasible(self) -> int:
        """
        Returns the number of feasible solutions in the population.

        Returns
        -------
        int
            Number of feasible solutions.
        """
        return len(self._feas)

    def num_infeasible(self) -> int:
        """
        Returns the number of infeasible solutions in the population.

        Returns
        -------
        int
            Number of infeasible solutions.
        """
        return len(self._infeas)

    def add(self, solution: Solution, cost_evaluator: CostEvaluator):
        """
        Adds the given solution to the population. Survivor selection is
        automatically triggered when the population reaches its maximum size.

        Parameters
        ----------
        solution
            Solution to add to the population.
        cost_evaluator
            CostEvaluator to use to compute the cost.
        """
        if solution.num_clients() == 0:
            msg = """
            An empty solution is being added to the population. This typically
            indicates that there is a significant difference between the values
            of the prizes and the other objective terms, which hints at a data
            problem. Note that not every part of PyVRP can work gracefully with
            empty solutions.
            """
            warn(msg, EmptySolutionWarning)

        # Note: the CostEvaluator is required here since adding a solution
        # may trigger a purge which needs to compute the biased fitness which
        # requires computing the cost.
        if solution.is_feasible():
            # Note: the feasible subpopulation actually does not depend
            # on the penalty values but we use the same implementation.
            self._feas.add(solution, cost_evaluator)
        else:
            self._infeas.add(solution, cost_evaluator)

    def clear(self):
        """
        Clears the population by removing all solutions currently in the
        population.
        """
        self._feas = SubPopulation(self._op, self._params)
        self._infeas = SubPopulation(self._op, self._params)

    def select(
        self,
        rng: RandomNumberGenerator,
        cost_evaluator: CostEvaluator,
        k: int = 2,
    ) -> Tuple[Solution, Solution]:
        """
        Selects two (if possible non-identical) parents by tournament, subject
        to a diversity restriction.

        Parameters
        ----------
        rng
            Random number generator.
        cost_evaluator
            Cost evaluator to use when computing the fitness.
        k
            The number of solutions to draw for the tournament. Defaults to
            two, which results in a binary tournament.

        Returns
        -------
        tuple
            A solution pair (parents).
        """
        self._update_fitness(cost_evaluator)

        first = self._get_tournament(rng, k)
        second = self._get_tournament(rng, k)

        diversity = self._op(first, second)
        lb = self._params.lb_diversity
        ub = self._params.ub_diversity

        tries = 1
        while not (lb <= diversity <= ub) and tries <= 10:
            tries += 1
            second = self._get_tournament(rng, k)
            diversity = self._op(first, second)

        return first, second

    def get_tournament(
        self,
        rng: RandomNumberGenerator,
        cost_evaluator: CostEvaluator,
        k: int = 2,
    ) -> Solution:
        """
        Selects a solution from this population by k-ary tournament, based
        on the (internal) fitness values of the selected solutions.

        Parameters
        ----------
        rng
            Random number generator.
        cost_evaluator
            Cost evaluator to use when computing the fitness.
        k
            The number of solutions to draw for the tournament. Defaults to
            two, which results in a binary tournament.

        Returns
        -------
        Solution
            The selected solution.
        """
        self._update_fitness(cost_evaluator)
        return self._get_tournament(rng, k)

    def _get_tournament(self, rng: RandomNumberGenerator, k: int) -> Solution:
        if k <= 0:
            raise ValueError(f"Expected k > 0; got k = {k}.")

        def select():
            num_feas = len(self._feas)
            idx = rng.randint(len(self))

            if idx < num_feas:
                return self._feas[idx]

            return self._infeas[idx - num_feas]

        items = [select() for _ in range(k)]
        fittest = min(items, key=lambda item: item.fitness)
        return fittest.solution
