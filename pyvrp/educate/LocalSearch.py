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

    def run(self, individual: Individual, intensify: bool):
        # HACK We keep searching and intensifying to mimic the local search
        # implementation of HGS-CVRP and HGS-VRPTW
        # TODO separate load/export individual from c++ implementation
        # so we only need to do it once
        while True:
            self.search(individual)
            if not (intensify and self.intensify(individual)):
                # Return unless we succesfully intensified
                return

    def intensify(
        self, individual: Individual, overlapToleranceDegrees: int = 0
    ) -> bool:
        # Runs intensification on the individual and returns whether an
        # improvement was found (TODO let c++ return this)
        cost = individual.cost()
        self._ls.intensify(individual, overlapToleranceDegrees)
        return individual.cost() < cost

    def search(self, individual: Individual):
        # TODO return whether improvement was found
        self._ls.search(individual)
