from typing import Any, Dict, Iterator, List, Tuple

from ._ProblemData import ProblemData
from ._XorShift128 import XorShift128

class Route:
    """
    A simple Route class that contains the route plan and some statistics.
    """

    def __init__(self, data: ProblemData, plan: List[int]) -> None: ...
    def __getitem__(self, idx: int) -> int: ...
    def __iter__(self) -> Iterator[int]: ...
    def __len__(self) -> int: ...
    @property
    def demand(self) -> int:
        """
        Total client demand on this route.
        """
    @property
    def distance(self) -> int:
        """
        Total distance travelled on this route.
        """
    @property
    def duration(self) -> int:
        """
        Total route duration, including waiting time.
        """
    @property
    def plan(self) -> List[int]:
        """
        Route plan, as a list of clients.
        """
    @property
    def service(self) -> int:
        """
        Total duration of service on the route.
        """
    @property
    def time_warp(self) -> int:
        """
        Any time warp incurred along the route.
        """
    @property
    def wait(self) -> int:
        """
        Total waiting duration on this route.
        """

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
    def get_routes(self) -> List[Route]:
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
            A list of routes. The routes each start and end at the depot (0),
            but that is implicit: the depot is not part of the returned routes.
        """
    def has_excess_load(self) -> bool:
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
    def distance(self) -> int:
        """
        Returns the total distance over all routes.

        Returns
        -------
        int
            Total distance over all routes.
        """
    def excess_load(self) -> int:
        """
        Returns the total excess load over all routes.

        Returns
        -------
        int
            Total excess load over all routes.
        """
    def time_warp(self) -> int:
        """
        Returns the total time warp load over all routes.

        Returns
        -------
        int
            Total time warp over all routes.
        """
    def is_feasible(self) -> bool:
        """
        Whether this individual is feasible. This is a shorthand for checking
        :meth:`~has_excess_load` and :meth:`~has_time_warp` both return
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
