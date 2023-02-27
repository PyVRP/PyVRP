from typing import Iterator, List, Tuple

from pyvrp.Individual import Individual
from pyvrp.ProblemData import ProblemData

class PopulationParams:
    generation_size: int
    lb_diversity: float
    min_pop_size: int
    nb_close: int
    nb_elite: int
    ub_diversity: float
    def __init__(
        self,
        min_pop_size: int = ...,
        generation_size: int = ...,
        nb_elite: int = ...,
        nb_close: int = ...,
        lb_diversity: float = ...,
        ub_diversity: float = ...,
    ) -> None: ...
    @property
    def max_pop_size(self) -> int: ...

class SubPopulation:
    def __init__(
        self, data: ProblemData, diversity_op, params: PopulationParams
    ) -> None:
        """
        Creates a SubPopulation instance.

        This subpopulation manages a number individuals, and initiates survivor
        selection (purging) when their number grows large. A subpopulation's
        individuals (and metadata) can be accessed via indexing and iteration.
        Each individual is stored as a tuple of type ``_Item``, which stores
        the individual itself, a fitness score (higher is worse), and a list
        of proximity values to the other individuals in the subpopulation.

        Parameters
        ----------
        data
            Data object describing the problem to be solved.
        diversity_op
            Operator to use to determine pairwise diversity between solutions.
        params
            Population parameters.
        """
    def add(self, individual: Individual) -> None:
        """
        Adds the given individual to the subpopulation. Survivor selection is
        automatically triggered when the population reaches its maximum size.

        Parameters
        ----------
        individual
            Individual to add to the subpopulation.
        """
    def purge(self) -> None:
        """
        Performs survivor selection: individuals in the subpopulation are
        purged until the population is reduced to the ``min_pop_size``.
        Purging happens to duplicate solutions first, and then to solutions
        with high biased fitness.
        """
    def update_fitness(self) -> None:
        """
        Updates the biased fitness scores of individuals in the subpopulation.
        This fitness depends on the quality of the solution (based on its cost)
        and the diversity w.r.t. to other individuals in the subpopulation.
        """
    def __getitem__(self, idx: int) -> SubPopulationItem: ...
    def __iter__(self) -> Iterator[SubPopulationItem]: ...
    def __len__(self) -> int: ...

class SubPopulationItem:
    @property
    def fitness(self) -> float: ...
    @property
    def individual(self) -> Individual: ...
    def avg_distance_closest(self) -> float:
        """
        Determines the average distance of the individual wrapped by this item
        to a number of individuals that are most similar to it. This provides a
        measure of the relative 'diversity' of the wrapped individual.

        Returns
        -------
        float
            The average distance/diversity of the wrapped individual.
        """
