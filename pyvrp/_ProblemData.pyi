from typing import List, Tuple

from pyvrp._Matrix import Matrix

class Client:
    """
    Simple data object storing all client data as properties.

    Attributes
    ----------
    demand
        The amount this client's demanding.
    service_duration
        This client's service duration, that is, the amount of time we need to
        visit the client for.
    tw_early
        Earliest time at which we can visit this client.
    tw_late
        Latest time at which we can visit this client.
    x
        Horizontal coordinate of this client, that is, the 'x' part of the
        client's (x, y) location tuple.
    y
        Vertical coordinate of this client, that is, the 'y' part of the
        client's (x, y) location tuple.
    """

    demand: int
    service_duration: int
    tw_early: int
    tw_late: int
    x: int
    y: int

class ProblemData:
    """
    Creates a problem data instance. This instance contains all information
    needed to solve the vehicle routing problem.

    Parameters
    ----------
    coords
        Array of (x, y) coordinates. The first coordinate at index 0 is assumed
        to be the depot.
    demands
        Array of client demands. The demand at index 0 is assumed to be the
        depot's demand, and should be zero.
    nb_vehicles
        The number of vehicles in this problem instance.
    vehicle_cap
        Homogenous vehicle capacity for all vehicles in the problem instance.
    time_windows
        Array of (early, late) time windows. The time window at index 0 is
        assumed to be the depot's time window, and describes the overall time
        horizon.
    service_durations
        Array of service durations, that is, the length of time needed to
        service a customer upon visiting. The service duration at index 0 is
        assumed to be the depot's service time, and should be zero.
    distance_matrix
        A matrix that gives the distances between clients (and the depot at
        index 0).
    duration_matrix
        A matrix that gives the travel times between clients (and the depot at
        index 0).

    Notes
    -----
    All array data assume that the data at or involving index 0 relates to the
    depot, and all other indices specify client information.
    """

    def __init__(
        self,
        coords: List[Tuple[int, int]],
        demands: List[int],
        nb_vehicles: int,
        vehicle_cap: int,
        time_windows: List[Tuple[int, int]],
        service_durations: List[int],
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
    def num_vehicles(self) -> int:
        """
        Number of vehicles in this problem instance.

        Returns
        -------
        int
            Number of vehicles in the instance.
        """
    @property
    def vehicle_capacity(self) -> int:
        """
        Returns the homogenous vehicle capacities of all vehicles in this
        problem data instance.

        Returns
        -------
        int
            Capacity of each vehicle in the instance.
        """
