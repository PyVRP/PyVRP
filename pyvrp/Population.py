from __future__ import annotations

from typing import Callable, Tuple

from .Individual import Individual
from .PenaltyManager import PenaltyManager
from .ProblemData import ProblemData
from .SubPopulation import PopulationParams, SubPopulation
from .XorShift128 import XorShift128

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
        self._rng = rng
        self._op = lambda indiv1, indiv2: diversity_op(data, indiv1, indiv2)
        self._new_indiv = lambda: Individual(data, penalty_manager, rng)
        self._params = params

        self._feas = SubPopulation(self._op, params)
        self._infeas = SubPopulation(self._op, params)

        for _ in range(params.min_pop_size):
            self.add(self._new_indiv())

    @property
    def feasible_subpopulation(self) -> SubPopulation:
        """
        Returns the feasible subpopulation maintained by this population
        instance.

        Returns
        -------
        SubPopulation
            Feasible subpopulation.
        """
        return self._feas

    @property
    def infeasible_subpopulation(self) -> SubPopulation:
        """
        Returns the infeasible subpopulation maintained by this population
        instance.

        Returns
        -------
        SubPopulation
            Infeasible subpopulation.
        """
        return self._infeas

    def __len__(self) -> int:
        """
        Returns the current population size, that is, the size of its feasible
        and infeasible subpopulations.

        Returns
        -------
        int
            Population size.
        """
        return len(self._feas) + len(self._infeas)

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

        diversity = self._op(first, second)
        lb = self._params.lb_diversity
        ub = self._params.ub_diversity

        tries = 1
        while not (lb <= diversity <= ub) and tries <= 10:
            tries += 1
            second = self.get_binary_tournament()
            diversity = self._op(first, second)

        return first, second

    def restart(self):
        """
        Restarts the population. All individuals are removed and a new initial
        population population is generated.
        """
        self._feas = SubPopulation(self._op, self._params)
        self._infeas = SubPopulation(self._op, self._params)

        for _ in range(self._params.min_pop_size):
            self.add(self._new_indiv())

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
                return self._feas[idx], self._feas.get_biased_fitness(idx)

            return self._infeas[
                idx - num_feas
            ], self._infeas.get_biased_fitness(idx - num_feas)

        indiv1, fitness1 = select()
        indiv2, fitness2 = select()

        if fitness1 < fitness2:
            return indiv1

        return indiv2
