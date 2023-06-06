import pathlib
from dataclasses import dataclass
from typing import Callable, List, Optional, Union

from pyvrp.GeneticAlgorithm import GeneticAlgorithm
from pyvrp.PenaltyManager import PenaltyManager
from pyvrp.Population import Population, PopulationParams
from pyvrp.Result import Result
from pyvrp._Individual import Individual
from pyvrp._ProblemData import Client, ProblemData
from pyvrp._XorShift128 import XorShift128
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.educate import (
    NODE_OPERATORS,
    ROUTE_OPERATORS,
    LocalSearch,
    compute_neighbours,
)
from pyvrp.read import read
from pyvrp.stop import StoppingCriterion

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
        self._data: Optional[ProblemData] = None

    @property
    def data(self) -> Optional[ProblemData]:
        return self._data

    @classmethod
    def read(
        cls,
        where: Union[str, pathlib.Path],
        instance_format: str,
        round_func: Union[str, Callable],
    ) -> "Model":
        data = read(where, instance_format, round_func)
        clients = [data.client(idx) for idx in range(data.num_clients + 1)]
        edges = [
            Edge(
                clients[frm],
                clients[to],
                data.dist(frm, to),
                data.duration(frm, to),
            )
            for frm in range(data.num_clients + 1)
            for to in range(data.num_clients + 1)
        ]

        self = Model()
        self._clients = clients[1:]
        self._depots = clients[:1]
        self._edges = edges
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
            x,
            y,
            demand,
            service_duration,
            tw_early,
            tw_late,
            prize,
            required,
        )

        self._clients.append(client)
        return client

    def add_depot(
        self, x: int, y: int, tw_early: int = 0, tw_late: int = 0
    ) -> Depot:
        if len(self._depots) > 1:
            msg = "PyVRP does not yet support multi-depot VRPs."
            raise ValueError(msg)

        depot = Depot(x, y, tw_early=tw_early, tw_late=tw_late)
        self._depots.append(depot)
        return depot

    def add_edge(
        self,
        frm: Union[Client, Depot],
        to: Union[Client, Depot],
        distance: int,
        duration: int,
    ) -> Edge:
        if distance < 0 or duration < 0:
            raise ValueError("Cannot have negative edge distance or duration.")

        edge = Edge(frm, to, distance, duration)
        self._edges.append(edge)
        return edge

    def update(self):
        # TODO
        clients = self._depots + self._clients
        # client2idx = {id(client): idx for idx, client in enumerate(clients)}

        num_vehicles = 1
        vehicle_capacity = 1

        distances = [[]]
        durations = [[]]

        self._data = ProblemData(
            clients, num_vehicles, vehicle_capacity, distances, durations
        )

    def solve(self, stop: StoppingCriterion, seed: int = 0) -> Result:
        if self.data is None:
            self.update()
            assert self.data is not None  # mypy needs this assert

        rng = XorShift128(seed=seed)
        ls = LocalSearch(self.data, rng, compute_neighbours(self.data))

        for op in NODE_OPERATORS:
            ls.add_node_operator(op(self.data))

        for op in ROUTE_OPERATORS:
            ls.add_route_operator(op(self.data))

        pm = PenaltyManager()
        pop_params = PopulationParams()
        pop = Population(bpd, pop_params)
        init = [
            Individual.make_random(self.data, rng)
            for _ in range(pop_params.min_pop_size)
        ]

        algo = GeneticAlgorithm(self.data, pm, rng, pop, ls, srex, init)
        return algo.run(stop)
