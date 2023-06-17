from typing import Callable, Iterator

from pyvrp._CostEvaluator import CostEvaluator
from pyvrp._Solution import Solution

class PopulationParams:
    generation_size: int
    lb_diversity: float
    min_pop_size: int
    nb_close: int
    nb_elite: int
    ub_diversity: float
    def __init__(
        self,
        min_pop_size: int = 25,
        generation_size: int = 40,
        nb_elite: int = 4,
        nb_close: int = 5,
        lb_diversity: float = 0.1,
        ub_diversity: float = 0.5,
    ) -> None: ...
    @property
    def max_pop_size(self) -> int: ...

class SubPopulation:
    def __init__(
        self,
        diversity_op: Callable[[Solution, Solution], float],
        params: PopulationParams,
    ) -> None:
        """
        Creates a SubPopulation instance.

        This subpopulation manages a collection of solutions, and initiates
        survivor selection (purging) when their number grows large. A
        subpopulation's solutions can be accessed via indexing and iteration.
        Each solution is stored as a tuple of type ``_Item``, which stores
        the solution itself, a fitness score (higher is worse), and a list
        of proximity values to the other solutions in the subpopulation.

        Parameters
        ----------
        diversity_op
            Operator to use to determine pairwise diversity between solutions.
        params
            Population parameters.
        """
    def add(self, solution: Solution, cost_evaluator: CostEvaluator) -> None:
        """
        Adds the given solution to the subpopulation. Survivor selection is
        automatically triggered when the population reaches its maximum size.

        Parameters
        ----------
        solution
            Solution to add to the subpopulation.
        cost_evaluator
            CostEvaluator to use to compute the cost.
        """
    def purge(self, cost_evaluator: CostEvaluator) -> None:
        """
        Performs survivor selection: solutions in the subpopulation are
        purged until the population is reduced to the ``min_pop_size``.
        Purging happens to duplicate solutions first, and then to solutions
        with high biased fitness.

        Parameters
        ----------
        cost_evaluator
            CostEvaluator to use to compute the cost.
        """
    def update_fitness(self, cost_evaluator: CostEvaluator) -> None:
        """
        Updates the biased fitness scores of solutions in the subpopulation.
        This fitness depends on the quality of the solution (based on its cost)
        and the diversity w.r.t. to other solutions in the subpopulation.

        .. warning::

           This function must be called before accessing the
           :meth:`~SubPopulationItem.fitness` attribute.
        """
    def __getitem__(self, idx: int) -> SubPopulationItem: ...
    def __iter__(self) -> Iterator[SubPopulationItem]: ...
    def __len__(self) -> int: ...

class SubPopulationItem:
    @property
    def fitness(self) -> float:
        """
        Fitness value for this SubPopulationItem.

        Returns
        -------
        float
            Fitness value for this SubPopulationItem.

        .. warning::

           This is a cached property that is not automatically updated. Before
           accessing the property, :meth:`~SubPopulation.update_fitness` should
           be called unless the population has not changed since the last call.
        """
    @property
    def solution(self) -> Solution:
        """
        Solution for this SubPopulationItem.

        Returns
        -------
        Solution
            Solution for this SubPopulationItem.
        """
    def avg_distance_closest(self) -> float:
        """
        Determines the average distance of the solution wrapped by this item
        to a number of solutions that are most similar to it. This provides a
        measure of the relative 'diversity' of the wrapped solution.

        Returns
        -------
        float
            The average distance/diversity of the wrapped solution.
        """
