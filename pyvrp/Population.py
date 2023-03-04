from __future__ import annotations

from typing import Callable, Generator, List, Tuple

from ._Individual import Individual
from ._SubPopulation import PopulationParams, SubPopulation
from ._XorShift128 import XorShift128


class Population:
    """
    Creates a Population instance.

    Parameters
    ----------
    initial_solutions
        List of individuals used to initialize the population.
    diversity_op
        Operator to use to determine pairwise diversity between solutions. Have
        a look at :mod:`pyvrp.diversity` for available operators.
    params, optional
        Population parameters. If not provided, a default will be used.
    """

    def __init__(
        self,
        initial_solutions: List[Individual],
        diversity_op: Callable[[Individual, Individual], float],
        params: PopulationParams = PopulationParams(),
    ):
        self._op = diversity_op
        self._initial_solutions = initial_solutions
        self._params = params

        self._feas = SubPopulation(diversity_op, params)
        self._infeas = SubPopulation(diversity_op, params)

        for indiv in initial_solutions:
            self.add(indiv)

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

    def add(self, individual: Individual):
        """
        Adds the given individual to the population. Survivor selection is
        automatically triggered when the population reaches its maximum size.

        Parameters
        ----------
        individual
            Individual to add to the population.
        """
        if individual.is_feasible():
            self._feas.add(individual)
        else:
            self._infeas.add(individual)

    def select(self, rng: XorShift128) -> Tuple[Individual, Individual]:
        """
        Selects two (if possible non-identical) parents by binary tournament,
        subject to a diversity restriction.

        Parameters
        ----------
        rng
            Random number generator.

        Returns
        -------
        tuple
            A pair of individuals (parents).
        """
        first = self.get_binary_tournament(rng)
        second = self.get_binary_tournament(rng)

        diversity = self._op(first, second)
        lb = self._params.lb_diversity
        ub = self._params.ub_diversity

        tries = 1
        while not (lb <= diversity <= ub) and tries <= 10:
            tries += 1
            second = self.get_binary_tournament(rng)
            diversity = self._op(first, second)

        return first, second

    def restart(self):
        """
        Restarts the population. All current individuals are removed and the
        original initial individuals are used agains to initialize the
        restarted population.
        """
        self._feas = SubPopulation(self._op, self._params)
        self._infeas = SubPopulation(self._op, self._params)

        for indiv in self._initial_solutions:
            self.add(indiv)

    def get_binary_tournament(self, rng: XorShift128) -> Individual:
        """
        Selects an individual from this population by binary tournament.

        Parameters
        ----------
        rng
            Random number generator.

        Returns
        -------
        Individual
            The selected individual.
        """

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
