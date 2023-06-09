from typing import List

from pyvrp._Matrix import Matrix

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
        prize: int = 0,
        required: bool = True,
    ) -> None: ...

class RouteData:
    """
    Simple data object storing all route data as properties.

    Attributes
    ----------
    capacity
        Capacity of this route (maximum total demand that can be served).
    """

    capacity: int

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
    capacities
        List of capacities for all routes in the problem instance.
    duration_matrix
        A matrix that gives the travel times between clients (and the depot at
        index 0).
    """

    def __init__(
        self,
        clients: List[Client],
        capacities: List[int],
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
    def route_data(self, route: int) -> RouteData:
        """
        Returns route data for the given route index.

        Parameters
        ----------
        route
            Route number for which to retrieve information.

        Returns
        -------
        RouteData
            A simple data object containing the requested route's information.
        """
    def route_type(self, route: int) -> int:
        """
        Returns the route type (index) for the given route index.

        Parameters
        ----------
        route
            Route number for which to retrieve route type.

        Returns
        -------
        int
            Index of the route type of the requested route.
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
    def distance_matrix(self) -> Matrix:
        """
        Returns the travel distance matrix used for distance computations.

        Returns
        -------
        Matrix
            Travel distance matrix.
        """
    def duration_matrix(self) -> Matrix:
        """
        Returns the travel duration matrix used for duration computations.

        Returns
        -------
        Matrix
            Travel duration matrix.
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
    def max_num_routes(self) -> int:
        """
        Maximum number of routes in this problem instance.

        Returns
        -------
        int
            Maximum number of routes in the instance.
        """
