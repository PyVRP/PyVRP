from typing import Any, List, Tuple

class Client:
    def __init__(self, *args, **kwargs) -> None: ...
    @property
    def demand(self) -> int: ...
    @property
    def release_time(self) -> int: ...
    @property
    def serv_dur(self) -> int: ...
    @property
    def tw_early(self) -> int: ...
    @property
    def tw_late(self) -> int: ...
    @property
    def x(self) -> int: ...
    @property
    def y(self) -> int: ...

class ProblemData:
    def __init__(
        self,
        coords: List[Tuple[int, int]],
        demands: List[int],
        nb_vehicles: int,
        vehicle_cap: int,
        time_windows: List[Tuple[int, int]],
        service_durations: List[int],
        duration_matrix: List[List[int]],
        release_times: List[int],
    ) -> None: ...
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
    def dist(self, first: int, second: int) -> int: ...
    def distance_matrix(self, *args, **kwargs) -> Any: ...
    @staticmethod
    def from_file(where: str) -> ProblemData: ...
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
        pass
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
        pass
