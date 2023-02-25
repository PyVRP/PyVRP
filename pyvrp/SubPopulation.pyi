from typing import Callable, Iterator, List, Tuple

from pyvrp.Individual import Individual

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
        self,
        diversity_op: Callable[[Individual, Individual], float],
        params: PopulationParams,
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
    def avg_distance_closest(self, idx: int) -> float:
        """
        Determines the average distance of the individual at the given index to
        a number of individuals that are most similar to it. This provides a
        measure of the relative 'diversity' of this individual.

        Parameters
        ----------
        idx
            Index of the Individual whose average distance/diversity to
            calculate.

        Returns
        -------
        float
            The average distance/diversity of the given individual relative to
            the total subpopulation.
        """
    def get_biased_fitness(self, idx: int) -> float:
        """
        Returns the biased fitness of the individual at the given index,
         taking into account the diversity rank and fitness rank.

        Parameters
        ----------
        idx
            Index of the Individual whose biased fitness to return.

        Returns
        -------
        float
            The biased fitness of the individual at the given index.
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
    def _get_proximity(self, idx: int) -> List[Tuple[float, Individual]]:
        """
        Gets proximity structure for individual at index (for testing).
        """
    def __getitem__(self, idx: int) -> Individual: ...
    # TODO is explicit iterator faster?
    # def __iter__(self) -> Iterator[Individual]: ...
    def __len__(self) -> int: ...
