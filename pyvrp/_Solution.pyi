from typing import Any, Dict, Iterator, List, Tuple, Union

from ._ProblemData import ProblemData
from ._XorShift128 import XorShift128

class Route:
    """
    A simple class that stores the route plan and some statistics.
    """

    def __init__(
        self, data: ProblemData, visits: List[int], vehicle_type: int
    ) -> None: ...
    def __getitem__(self, idx: int) -> int: ...
    def __iter__(self) -> Iterator[int]: ...
    def __len__(self) -> int: ...
    def is_feasible(self) -> bool: ...
    def has_excess_load(self) -> bool: ...
    def has_time_warp(self) -> bool: ...
    def demand(self) -> int:
        """
        Total client demand on this route.
        """
    def excess_load(self) -> int:
        """
        Demand in excess of the vehicle's capacity.
        """
    def distance(self) -> int:
        """
        Total distance travelled on this route.
        """
    def duration(self) -> int:
        """
        Total route duration, including waiting time.
        """
    def visits(self) -> List[int]:
        """
        Route visits, as a list of clients.
        """
    def service_duration(self) -> int:
        """
        Total duration of service on the route.
        """
    def time_warp(self) -> int:
        """
        Any time warp incurred along the route.
        """
    def wait_duration(self) -> int:
        """
        Total waiting duration on this route.
        """
    def release_time(self) -> int:
        """
        Release time of visits on this route.
        """
    def prizes(self) -> int:
        """
        Total prize value collected on this route.
        """
    def centroid(self) -> Tuple[float, float]:
        """
        Center point of the client locations on this route.
        """
    def vehicle_type(self) -> int:
        """
        Index of the type of vehicle used on this route.
        """

class Solution:
    """
    Encodes VRP solutions.

    Parameters
    ----------
    data
        Data instance.
    routes
        Route list to use. Should be a list of Route objects, or list of list
        of ints representing clients.

    Raises
    ------
    RuntimeError
        When the number of routes in the ``routes`` argument exceeds
        :py:attr:`~pyvrp._ProblemData.ProblemData.num_vehicles`, or when an
        empty route has been passed as part of ``routes``.
    """

    def __init__(
        self,
        data: ProblemData,
        routes: List[Union[Route, List[int]]],
    ) -> None: ...
    @classmethod
    def make_random(cls, data: ProblemData, rng: XorShift128) -> Solution:
        """
        Creates a randomly generated solution.

        Parameters
        ----------
        data
            Data instance.
        rng
            Random number generator to use.

        Returns
        -------
        Solution
            The randomly generated solution.
        """
    def get_neighbours(self) -> List[Tuple[int, int]]:
        """
        Returns a list of neighbours for each client, by index. Also includes
        the depot at index 0, which only neighbours itself.

        Returns
        -------
        list
            A list of ``(pred, succ)`` tuples that encode for each client their
            predecessor and successors in this solutions's routes.
        """
    def get_routes(self) -> List[Route]:
        """
        The solution's routing decisions.

        .. note::

           The length of this list is equal to the number of non-empty routes,
           which is at most equal to
           :py:attr:`~pyvrp._ProblemData.ProblemData.num_vehicles`.

        Returns
        -------
        list
            A list of routes. Each :class:`~pyvrp._Solution.Route` starts and
            ends at the depot (0), but that is implicit: the depot is not part
            of the returned routes.
        """
    def has_excess_load(self) -> bool:
        """
        Returns whether this solution violates capacity constraints.

        Returns
        -------
        bool
            True if the solution is not capacity feasible, False otherwise.
        """
    def has_time_warp(self) -> bool:
        """
        Returns whether this solution violates time window constraints.

        Returns
        -------
        bool
            True if the solution is not time window feasible, False
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
    def prizes(self) -> int:
        """
        Returns the total collected prize value over all routes.

        Returns
        -------
        int
            Value of collected prizes.
        """
    def uncollected_prizes(self) -> int:
        """
        Total prize value of all clients not visited in this solution.

        Returns
        -------
        int
            Value of uncollected prizes.
        """
    def is_feasible(self) -> bool:
        """
        Whether this solution is feasible. This is a shorthand for checking
        that :meth:`~has_excess_load` and :meth:`~has_time_warp` both return
        false.

        Returns
        -------
        bool
            Whether the solution of this solution is feasible with respect to
            capacity and time window constraints.
        """
    def num_routes(self) -> int:
        """
        Number of routes in this solution.

        Returns
        -------
        int
            Number of routes.
        """
    def num_clients(self) -> int:
        """
        Number of clients in this solution.

        Returns
        -------
        int
            Number of clients in this solution.
        """
    def __copy__(self) -> Solution: ...
    def __deepcopy__(self, memo: Dict) -> Solution: ...
    def __hash__(self) -> int: ...
    def __eq__(self, other: Any) -> bool: ...
    def __str__(self) -> str: ...
