from typing import List

from pyvrp.Individual import Individual
from pyvrp.PenaltyManager import PenaltyManager
from pyvrp.ProblemData import ProblemData
from pyvrp.XorShift128 import XorShift128
from ._LocalSearch import LocalSearch as _LocalSearch

Neighbours = List[List[int]]


class LocalSearch:
    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: XorShift128,
        neighbours: Neighbours,
    ) -> None:
        """
        Python wrapper for a LocalSearch instance.

        Parameters
        ----------
        data
            Data object describing the problem to be solved.
        penalty_manager
            Penalty manager to use.
        rng
            Random number generator.
        neighbours
            Matrix that defines the local search neighbourhood.
        """
        self._ls = _LocalSearch(data, penalty_manager, rng, neighbours)

    def add_node_operator(self, op) -> None:
        self._ls.add_node_operator(op)

    def add_route_operator(self, op) -> None:
        self._ls.add_route_operator(op)

    def set_neighbours(self, neighbours: Neighbours) -> None:
        self._ls.set_neighbours(neighbours)

    def get_neighbours(self) -> Neighbours:
        return self._ls.get_neighbours()

    def intensify(self, individual: Individual) -> None:
        self._ls.intensify(individual)

    def search(self, individual: Individual) -> None:
        self._ls.search(individual)
