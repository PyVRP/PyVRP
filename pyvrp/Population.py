from __future__ import annotations

from typing import Callable, Generator, Tuple

from ._CostEvaluator import CostEvaluator
from ._Individual import Individual
from ._SubPopulation import PopulationParams, SubPopulation
from ._XorShift128 import XorShift128


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
            CostEvaluator to use to compute the cost. Required here
            since adding an individual may trigger a purge which needs to
            compute the biased fitness which requires computing the cost.
        """
        if individual.is_feasible():
            # Note: the feasible subpopulation actually doet not depend
            # on the penalty values but we use the same implementation.
            self._feas.add(individual, cost_evaluator)
        else:
            self._infeas.add(individual, cost_evaluator)

    def select(
        self,
        rng: XorShift128,
        cost_evaluator: CostEvaluator,
        update_fitness: bool = True,
    ) -> Tuple[Individual, Individual]:
        """
        Selects two (if possible non-identical) parents by binary tournament,
        subject to a diversity restriction.

        Parameters
        ----------
        rng
            Random number generator.
        cost_evaluator
            Cost evaluator to use when computing the fitness.
        update_fitness
            Boolean whether to update the fitness values before selecting the
            parents. Can be set to False for efficiency if it is known that the
            fitness has not changed, e.g. when calling select repeatedly.

        Returns
        -------
        tuple
            A pair of individuals (parents).
        """
        first = self.get_binary_tournament(rng, cost_evaluator, update_fitness)
        second = self.get_binary_tournament(rng, cost_evaluator, False)

        diversity = self._op(first, second)
        lb = self._params.lb_diversity
        ub = self._params.ub_diversity

        tries = 1
        while not (lb <= diversity <= ub) and tries <= 10:
            tries += 1
            second = self.get_binary_tournament(rng, cost_evaluator, False)
            diversity = self._op(first, second)

        return first, second

    def clear(self):
        """
        Clears the population by removing all individuals currently in the
        population.
        """
        self._feas = SubPopulation(self._op, self._params)
        self._infeas = SubPopulation(self._op, self._params)

    def get_binary_tournament(
        self,
        rng: XorShift128,
        cost_evaluator: CostEvaluator,
        update_fitness: bool = True,
    ) -> Individual:
        """
        Selects an individual from this population by binary tournament.

        Parameters
        ----------
        rng
            Random number generator.
        cost_evaluator
            Cost evaluator to use when computing the fitness.
        update_fitness
            Boolean whether to update the fitness values before getting the
            binary tournament. Can be set to False for efficiency if it is
            known that the fitness has not changed, e.g. when calling this
            function repeatedly.

        Returns
        -------
        Individual
            The selected individual.
        """

        if update_fitness:
            # Note: even though the cost does not change, for the feasible
            # subpopulation, we must always call compute_fitness as it is not
            # updated when adding individuals
            self._feas.update_fitness(cost_evaluator)
            self._infeas.update_fitness(cost_evaluator)

        def select():
            num_feas = len(self._feas)
            idx = rng.randint(len(self))

            if idx < num_feas:
                return self._feas[idx]

            return self._infeas[idx - num_feas]

        item1 = select()
        item2 = select()

        if item1.fitness < item2.fitness:
            return item1.individual

        return item2.individual
