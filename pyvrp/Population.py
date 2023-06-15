from __future__ import annotations

from typing import Callable, Generator, Tuple
from warnings import warn

from ._CostEvaluator import CostEvaluator
from ._Individual import Individual
from ._SubPopulation import PopulationParams, SubPopulation
from ._XorShift128 import XorShift128
from .exceptions import EmptySolutionWarning


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
        diversity_op: Callable[[Individual, Individual], float],
        params: PopulationParams = PopulationParams(),
    ):
        self._op = diversity_op
        self._params = params

        self._feas = SubPopulation(diversity_op, params)
        self._infeas = SubPopulation(diversity_op, params)

    def __iter__(self) -> Generator[Individual, None, None]:
        """
        Iterates over the individuals contained in this population.

        Yields
        ------
        iterable
            An iterable object of individuals.
        """
        for item in self._feas:
            yield item.individual

        for item in self._infeas:
            yield item.individual

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
        Returns the number of feasible individuals in the population.

        Returns
        -------
        int
            Number of feasible individuals.
        """
        return len(self._feas)

    def num_infeasible(self) -> int:
        """
        Returns the number of infeasible individuals in the population.

        Returns
        -------
        int
            Number of infeasible individuals.
        """
        return len(self._infeas)

    def add(self, individual: Individual, cost_evaluator: CostEvaluator):
        """
        Adds the given individual to the population. Survivor selection is
        automatically triggered when the population reaches its maximum size.

        Parameters
        ----------
        individual
            Individual to add to the population.
        cost_evaluator
            CostEvaluator to use to compute the cost.
        """
        if individual.num_clients() == 0:
            msg = """
            An empty solution is being added to the population. This typically
            indicates that there is a significant difference between the values
            of the prizes and the other objective terms, which hints at a data
            problem. Note that not every part of PyVRP can work gracefully with
            empty solutions.
            """
            warn(msg, EmptySolutionWarning)

        # Note: the CostEvaluator is required here since adding an individual
        # may trigger a purge which needs to compute the biased fitness which
        # requires computing the cost.
        if individual.is_feasible():
            # Note: the feasible subpopulation actually doet not depend
            # on the penalty values but we use the same implementation.
            self._feas.add(individual, cost_evaluator)
        else:
            self._infeas.add(individual, cost_evaluator)

    def clear(self):
        """
        Clears the population by removing all individuals currently in the
        population.
        """
        self._feas = SubPopulation(self._op, self._params)
        self._infeas = SubPopulation(self._op, self._params)

    def select(
        self,
        rng: XorShift128,
        cost_evaluator: CostEvaluator,
        k: int = 2,
    ) -> Tuple[Individual, Individual]:
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
            The number of individuals to draw for the tournament. Defaults to
            two, which results in a binary tournament.

        Returns
        -------
        tuple
            A pair of individuals (parents).
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
        self, rng: XorShift128, cost_evaluator: CostEvaluator, k: int = 2
    ) -> Individual:
        """
        Selects an individual from this population by k-ary tournament, based
        on the (internal) fitness values of the selected individuals.

        Parameters
        ----------
        rng
            Random number generator.
        cost_evaluator
            Cost evaluator to use when computing the fitness.
        k
            The number of individuals to draw for the tournament. Defaults to
            two, which results in a binary tournament.

        Returns
        -------
        Individual
            The selected individual.
        """
        self._update_fitness(cost_evaluator)
        return self._get_tournament(rng, k)

    def _get_tournament(self, rng: XorShift128, k: int) -> Individual:
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
        return fittest.individual
