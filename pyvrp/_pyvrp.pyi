from typing import Callable, Iterator, List, Tuple, Union, overload

class CostEvaluator:
    """
    Creates a CostEvaluator instance.

    This class contains time warp and load penalties, and can compute penalties
    for a given time warp and load.

    Parameters
    ----------
    capacity_penalty
        The penalty for each unit of excess load over the vehicle capacity.
    tw_penalty
        The penalty for each unit of time warp.
    """

    def __init__(
        self, capacity_penalty: int = 0, tw_penalty: int = 0
    ) -> None: ...
    def load_penalty(self, load: int, vehicle_capacity: int) -> int: ...
    def tw_penalty(self, time_warp: int) -> int: ...
    def penalised_cost(self, solution: Solution) -> int: ...
    def cost(self, solution: Solution) -> int:
        """
        Evaluates and returns the cost/objective of the given solution.
        Hand-waving some details, let :math:`x_{ij} \\in \\{ 0, 1 \\}` indicate
        if edge :math:`(i, j)` is used in the solution encoded by the given
        solution, and :math:`y_i \\in \\{ 0, 1 \\}` indicate if client
        :math:`i` is visited. The objective is then given by

        .. math::

           \\sum_{(i, j)} d_{ij} x_{ij} + \\sum_{i} p_i (1 - y_i),

        where the first part lists the distance costs, and the second part the
        prizes of the unvisited clients.
        """

class DynamicBitset:
    def __init__(self, num_bits: int) -> None: ...
    def __eq__(self, other: object) -> bool: ...
    def count(self) -> int: ...
    def __len__(self) -> int: ...
    def __getitem__(self, idx: int) -> bool: ...
    def __setitem__(self, idx: int, value: bool) -> None: ...
    def __or__(self, other: DynamicBitset) -> DynamicBitset: ...
    def __and__(self, other: DynamicBitset) -> DynamicBitset: ...
    def __xor__(self, other: DynamicBitset) -> DynamicBitset: ...
    def __invert__(self) -> DynamicBitset: ...

class Matrix:
    @overload
    def __init__(self, dimension: int) -> None: ...
    @overload
    def __init__(self, n_rows: int, n_cols: int) -> None: ...
    @overload
    def __init__(self, data: List[List[int]]) -> None: ...
    @property
    def num_cols(self) -> int: ...
    @property
    def num_rows(self) -> int: ...
    def max(self) -> int: ...
    def size(self) -> int: ...
    def __getitem__(self, idx: Tuple[int, int]) -> int: ...
    def __setitem__(self, idx: Tuple[int, int], value: int) -> None: ...

class Client:
    """
    Simple data object storing all client data as (read-only) properties.

    Parameters
    ----------
    x
        Horizontal coordinate of this client, that is, the 'x' part of the
        client's (x, y) location tuple.
    y
        Vertical coordinate of this client, that is, the 'y' part of the
        client's (x, y) location tuple.
    demand
        The amount this client's demanding. Default 0.
    service_duration
        This client's service duration, that is, the amount of time we need to
        visit the client for. Service should start (but not necessarily end)
        within the [:py:attr:`~tw_early`, :py:attr:`~tw_late`] interval.
        Default 0.
    tw_early
        Earliest time at which we can visit this client. Default 0.
    tw_late
        Latest time at which we can visit this client. Default 0.
    release_time
        Earliest time at which this client is released, that is, the earliest
        time at which a vehicle may leave the depot to visit this client.
        Default 0.
    prize
        Prize collected by visiting this client. Default 0.
    required
        Whether this client must be part of a feasible solution. Default True.
    """

    x: int
    y: int
    demand: int
    service_duration: int
    tw_early: int
    tw_late: int
    release_time: int
    prize: int
    required: bool
    def __init__(
        self,
        x: int,
        y: int,
        demand: int = 0,
        service_duration: int = 0,
        tw_early: int = 0,
        tw_late: int = 0,
        release_time: int = 0,
        prize: int = 0,
        required: bool = True,
    ) -> None: ...

class VehicleType:
    """
    Simple data object storing all vehicle type data as properties.

    Attributes
    ----------
    capacity
        Capacity (maximum total demand) of this vehicle type.
    num_available
        Number of vehicles of this type that are available.
    start_depot
        Depot from which these vehicles depart.
    end_depot
        Depot to which these vehicles return.
    """

    capacity: int
    num_available: int
    start_depot: int
    end_depot: int
    def __init__(
        self,
        capacity: int,
        num_available: int,
        start_depot: int = 0,
        end_depot: int = 0,
    ) -> None: ...

class ProblemData:
    """
    Creates a problem data instance. This instance contains all information
    needed to solve the vehicle routing problem.

    Parameters
    ----------
    clients
        List of clients. The first client (at index 0) is assumed to be the
        depot. The time window for the depot is assumed to describe the overall
        time horizon. The depot should have 0 demand and 0 service duration.
    vehicle_types
        List of vehicle types in the problem instance.
    duration_matrix
        A matrix that gives the travel times between clients (and the depot at
        index 0).
    """

    def __init__(
        self,
        clients: List[Client],
        vehicle_types: List[VehicleType],
        distance_matrix: List[List[int]],
        duration_matrix: List[List[int]],
    ): ...
    def client(self, client: int) -> Client:
        """
        Returns client data for the given client.

        Parameters
        ----------
        client
            Client number whose information to retrieve.

        Returns
        -------
        Client
            A simple data object containing the requested client's information.
        """
    def depot(self) -> Client:
        """
        Returns 'client' information for the depot, which is stored internally
        as the client with number `0`.

        Returns
        -------
        Client
            A simple data object containing the depot's information.
        """
    def centroid(self) -> Tuple[float, float]:
        """
        Center point of all client locations (excluding the depot).

        Returns
        -------
        tuple
            Centroid of all client locations.
        """
    def vehicle_type(self, vehicle_type: int) -> VehicleType:
        """
        Returns vehicle type data for the given vehicle type.

        Parameters
        ----------
        vehicle_type
            Vehicle type number whose information to retrieve.

        Returns
        -------
        VehicleType
            A simple data object containing the vehicle type information.
        """
    def dist(self, first: int, second: int) -> int:
        """
        Returns the travel distance between the first and second argument,
        according to this instance's travel distance matrix.

        Parameters
        ----------
        first
            Client or depot number.
        second
            Client or depot number.

        Returns
        -------
        int
            Travel distance between the given clients.
        """
    def duration(self, first: int, second: int) -> int:
        """
        Returns the travel duration between the first and second argument,
        according to this instance's travel duration matrix.

        Parameters
        ----------
        first
            Client or depot number.
        second
            Client or depot number.

        Returns
        -------
        int
            Travel duration between the given clients.
        """
    @property
    def num_clients(self) -> int:
        """
        Number of clients in this problem instance.

        Returns
        -------
        int
            Number of clients in the instance.
        """
    @property
    def num_vehicles(self) -> int:
        """
        Number of vehicles in this problem instance.

        Returns
        -------
        int
            Number of vehicles in this problem instance.
        """
    @property
    def num_vehicle_types(self) -> int:
        """
        Number of vehicle types in this problem instance.

        Returns
        -------
        int
            Number of vehicle types in this problem instance.
        """

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
        Route list to use. Can be a list of :class:`~Route` objects, or a lists
        of client visits. In case of the latter, all routes are assigned
        vehicles of the first type. That need not be a feasible assignment!

    Raises
    ------
    RuntimeError
        When the number of routes in the ``routes`` argument exceeds
        :py:attr:`~ProblemData.num_vehicles`, when an empty route has been
        passed as part of ``routes``, or when too many vehicles of a particular
        type have been used.
    """

    def __init__(
        self,
        data: ProblemData,
        routes: Union[List[Route], List[List[int]]],
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
           which is at most equal to :py:attr:`~ProblemData.num_vehicles`.

        Returns
        -------
        list
            A list of routes. Each :class:`~Route` starts and ends at the depot
            (0), but that is implicit: the depot is not part of the returned
            routes.
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
    def __deepcopy__(self, memo: dict) -> Solution: ...
    def __hash__(self) -> int: ...
    def __eq__(self, other: object) -> bool: ...
    def __str__(self) -> str: ...

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

class TimeWindowSegment:
    def __init__(
        self,
        idx_first: int,
        idx_last: int,
        duration: int,
        time_warp: int,
        tw_early: int,
        tw_late: int,
        release_time: int,
    ) -> None:
        """
        Creates a time window segment.

        Parameters
        ----------
        idx_first
            Index of the first client in the route segment.
        idx_last
            Index of the last client in the route segment.
        duration
            Total duration, including waiting time.
        time_warp
            Total time warp on the route segment.
        tw_early
            Earliest visit moment of the first client.
        tw_late
            Latest visit moment of the first client.
        release_time
            Earliest moment to start the route segment.
        """
    @overload
    @staticmethod
    def merge(
        duration_matrix: Matrix,
        first: TimeWindowSegment,
        second: TimeWindowSegment,
    ) -> TimeWindowSegment:
        """
        Merges two time window segments, in order.
        """
    @overload
    @staticmethod
    def merge(
        duration_matrix: Matrix,
        first: TimeWindowSegment,
        second: TimeWindowSegment,
        third: TimeWindowSegment,
    ) -> TimeWindowSegment:
        """
        Merges three time window segments, in order.
        """
    def total_time_warp(self) -> int:
        """
        Returns the total time warp on this route segment.

        Returns
        -------
        int
            Total time warp on this route segment.
        """

class XorShift128:
    def __init__(self, seed: int) -> None: ...
    @staticmethod
    def max() -> int: ...
    @staticmethod
    def min() -> int: ...
    def rand(self) -> float: ...
    def randint(self, high: int) -> int: ...
    def __call__(self) -> int: ...
