from typing import Any, Dict, List, Tuple

from ._CostEvaluator import CostEvaluator
from ._ProblemData import ProblemData
from ._XorShift128 import XorShift128

class Individual:
    """
    The Individual class encodes VRP solutions.

    Parameters
    ----------
    data
        Data instance.
    routes
        Route list to use.

    Raises
    ------
    RuntimeError
        When the number of routes in the ``routes`` argument exceeds
        :py:attr:`~pyvrp._ProblemData.ProblemData.num_vehicles`.
    """

    def __init__(
        self,
        data: ProblemData,
        routes: List[List[int]],
    ) -> None: ...
    @classmethod
    def make_random(
        cls,
        data: ProblemData,
        rng: XorShift128,
    ) -> Individual:
        """
        Creates a randomly generated Individual.

        Parameters
        ----------
        data
            Data instance.
        rng
            Random number generator to use.

        Returns
        -------
        Individual
            The randomly generated Individual.
        """
    def cost(self, cost_evaluator: CostEvaluator) -> int:
        """
        Returns the current cost of the individual's solution.

        .. note::

           These costs depend on penalty values which is why a
           :class:`~pyvrp._CostEvaluator.CostEvaluator` object is required.

        Parameters
        ----------
        cost_evaluator
            CostEvaluator used to compute the cost.

        Returns
        -------
        int
            The current cost of this individual.
        """
    def get_neighbours(self) -> List[Tuple[int, int]]:
        """
        Returns a list of neighbours for each client, by index. Also includes
        the depot at index 0, which only neighbours itself.

        Returns
        -------
        list
            A list of ``(pred, succ)`` tuples that encode for each client their
            predecessor and successors in this individual's routes.
        """
    def get_routes(self) -> List[List[int]]:
        """
        The solution this individual encodes, as a list of routes.

        .. note::

           This list is of length
           :py:attr:`~pyvrp._ProblemData.ProblemData.num_vehicles`, but there
           could be a number of empty routes. These empty routes are all in the
           higher indices (guarantee). Use :meth:`~num_routes` to determine
           which of the lower indices contain non-empty routes.

        Returns
        -------
        list
            A list of routes, where each route is a list of client numbers. The
            routes each start and end at the depot (0), but that is implicit:
            the depot is not part of the returned routes.
        """
    def has_excess_capacity(self) -> bool:
        """
        Returns whether this individual violates capacity constraints.

        Returns
        -------
        bool
            True if the individual is not capacity feasible, False otherwise.
        """
    def has_time_warp(self) -> bool:
        """
        Returns whether this individual violates time window constraints.

        Returns
        -------
        bool
            True if the individual is not time window feasible, False
            otherwise.
        """
    def is_feasible(self) -> bool:
        """
        Whether this individual is feasible. This is a shorthand for checking
        :meth:`~has_excess_capacity` and :meth:`~has_time_warp` both return
        false.

        Returns
        -------
        bool
            Whether the solution of this individual is feasible with respect to
            capacity and time window constraints.
        """
    def num_routes(self) -> int:
        """
        Number of non-empty routes in this solution.

        Returns
        -------
        int
            Number of non-empty routes.
        """
    def __copy__(self) -> Individual: ...
    def __deepcopy__(self, memo: Dict) -> Individual: ...
    def __hash__(self) -> int: ...
    def __eq__(self, other: Any) -> bool: ...
    def __str__(self) -> str: ...
