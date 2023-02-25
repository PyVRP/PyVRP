from __future__ import annotations

from typing import Callable, Generator, Tuple

from ._Individual import Individual
from ._PenaltyManager import PenaltyManager
from ._ProblemData import ProblemData
from ._SubPopulation import PopulationParams, SubPopulation
from ._XorShift128 import XorShift128

_DiversityMeasure = Callable[[ProblemData, Individual, Individual], float]


class Population:
    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: XorShift128,
        diversity_op: _DiversityMeasure,
        params: PopulationParams = PopulationParams(),
    ):
        """
        Creates a Population instance.

        Parameters
        ----------
        data
            Data object describing the problem to be solved.
        penalty_manager
            Penalty manager to use.
        rng
            Random number generator.
        diversity_op
            Operator to use to determine pairwise diversity between solutions.
        params, optional
            Population parameters. If not provided, a default will be used.
        """
        self._data = data
        self._pm = penalty_manager
        self._rng = rng
        self._op = diversity_op
        self._params = params

        self._feas = SubPopulation(data, diversity_op, params)
        self._infeas = SubPopulation(data, diversity_op, params)

        for _ in range(params.min_pop_size):
            self.add(Individual(data, penalty_manager, rng))

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

    def select(self) -> Tuple[Individual, Individual]:
        """
        Selects two (if possible non-identical) parents by binary tournament,
        subject to a diversity restriction.

        Returns
        -------
        tuple
            A pair of individuals (parents).
        """
        first = self.get_binary_tournament()
        second = self.get_binary_tournament()

        diversity = self._op(self._data, first, second)
        lb = self._params.lb_diversity
        ub = self._params.ub_diversity

        tries = 1
        while not (lb <= diversity <= ub) and tries <= 10:
            tries += 1
            second = self.get_binary_tournament()
            diversity = self._op(self._data, first, second)

        return first, second

    def restart(self):
        """
        Restarts the population. All individuals are removed and a new initial
        population population is generated.
        """
        self._feas = SubPopulation(self._data, self._op, self._params)
        self._infeas = SubPopulation(self._data, self._op, self._params)

        for _ in range(self._params.min_pop_size):
            self.add(Individual(self._data, self._pm, self._rng))

    def get_binary_tournament(self) -> Individual:
        """
        Selects an individual from this population by binary tournament.

        Returns
        -------
        Individual
            The selected individual.
        """

        def select():
            num_feas = len(self._feas)
            idx = self._rng.randint(len(self))

            if idx < num_feas:
                return self._feas[idx]

            return self._infeas[idx - num_feas]

        item1 = select()
        item2 = select()

        if item1.fitness < item2.fitness:
            return item1.individual

        return item2.individual
