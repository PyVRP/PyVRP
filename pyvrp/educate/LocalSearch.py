from typing import List

from pyvrp._Individual import Individual
from pyvrp._PenaltyManager import PenaltyManager
from pyvrp._ProblemData import ProblemData
from pyvrp._XorShift128 import XorShift128

from ._LocalSearch import LocalSearch as _LocalSearch

Neighbours = List[List[int]]


class LocalSearch:
    def __init__(
        self,
        data: ProblemData,
        penalty_manager: PenaltyManager,
        rng: XorShift128,
        neighbours: Neighbours,
    ):
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
            List of lists that defines the local search neighbourhood.
        """
        self._ls = _LocalSearch(data, penalty_manager, rng, neighbours)

    def add_node_operator(self, op):
        self._ls.add_node_operator(op)

    def add_route_operator(self, op):
        self._ls.add_route_operator(op)

    def set_neighbours(self, neighbours: Neighbours):
        self._ls.set_neighbours(neighbours)

    def get_neighbours(self) -> Neighbours:
        return self._ls.get_neighbours()

    def run(self, individual: Individual, should_intensify: bool):
        # HACK We keep searching and intensifying to mimic the local search
        # implementation of HGS-CVRP and HGS-VRPTW
        # TODO separate load/export individual from c++ implementation
        # so we only need to do it once
        while True:
            individual = self.search(individual)

            if not should_intensify:
                return individual

            new_individual = self.intensify(individual)

            if new_individual.cost() < individual.cost():
                individual = new_individual
                continue

            return individual

    def intensify(
        self, individual: Individual, overlap_tolerance_degrees: int = 0
    ) -> Individual:
        return self._ls.intensify(individual, overlap_tolerance_degrees)

    def search(self, individual: Individual) -> Individual:
        return self._ls.search(individual)
