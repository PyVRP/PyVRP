import pathlib
from dataclasses import dataclass
from typing import Callable, List, Optional, Union

from pyvrp._ProblemData import Client, ProblemData
from pyvrp.read import read

Depot = Client


@dataclass
class Edge:
    frm: Client
    to: Client
    distance: int
    duration: int


class Model:
    """
    A modelling interface for vehicle routing problems.
    """

    def __init__(self):
        self._clients: List[Client] = []
        self._depots: List[Depot] = []
        self._edges: List[Edge] = []
        self._obj2idx = {}
        self._data: Optional[ProblemData] = None

    @classmethod
    def read(
        cls,
        where: Union[str, pathlib.Path],
        instance_format: str,
        round_func: Union[str, Callable],
    ) -> "Model":
        data = read(where, instance_format, round_func)

        self = Model()
        self._clients = [
            data.client(idx + 1) for idx in range(data.num_clients)
        ]
        self._depots = [data.client(0)]
        self._data = data

        return self

    def add_client(
        self,
        x: int,
        y: int,
        demand: int = 0,
        service_duration: int = 0,
        tw_early: int = 0,
        tw_late: int = 0,
        prize: int = 0,
        required: bool = True,
    ) -> Client:
        client = Client(
            x, y, demand, service_duration, tw_early, tw_late, prize, required
        )

        self._clients.append(client)
        self._obj2idx[id(client)] = len(self._clients) - 1
        return client

    def add_depot(
        self, x: int, y: int, tw_early: int = 0, tw_late: int = 0
    ) -> Depot:
        if len(self._depots) > 1:
            msg = "PyVRP does not yet support multi-depot VRPs."
            raise ValueError(msg)

        depot = Depot(x, y, tw_early=tw_early, tw_late=tw_late)
        self._depots.append(depot)
        self._obj2idx[id(depot)] = len(self._depots) - 1
        return depot

    def add_edge(
        self,
        frm: Union[Client, Depot],
        to: Union[Client, Depot],
        distance: int,
        duration: int,
    ) -> Edge:
        edge = Edge(frm, to, distance, duration)
        self._edges.append(edge)
        self._obj2idx[id(edge)] = len(self._edges) - 1
        return edge

    def update(self):
        pass

    def solve(self):
        if self._data is None:
            self.update()

        # TODO
