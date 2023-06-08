import pathlib
from dataclasses import dataclass
from typing import Callable, List, Optional, Union
from warnings import warn

import numpy as np

from pyvrp.GeneticAlgorithm import GeneticAlgorithm
from pyvrp.PenaltyManager import PenaltyManager
from pyvrp.Population import Population, PopulationParams
from pyvrp.Result import Result
from pyvrp._Individual import Individual
from pyvrp._ProblemData import Client, ProblemData
from pyvrp._XorShift128 import XorShift128
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.exceptions import ScalingWarning
from pyvrp.read import no_rounding, read
from pyvrp.stop import StoppingCriterion

Depot = Client


@dataclass
class Edge:
    frm: Client
    to: Client
    distance: int
    duration: int


@dataclass
class VehicleType:
    number: int
    capacity: int


class Model:
    """
    A simple interface for modelling vehicle routing problems with PyVRP.
    """

    def __init__(self):
        self._clients: List[Client] = []
        self._depots: List[Depot] = []
        self._edges: List[Edge] = []
        self._vehicle_types: List[VehicleType] = []
        self._data: Optional[ProblemData] = None

    @property
    def data(self) -> Optional[ProblemData]:
        """
        Returns the underlying :class:`~pyvrp._ProblemData.ProblemData`
        instance, if it already exists. Returns ``None`` if it has not
        yet been created.
        """
        return self._data

    @classmethod
    def read(
        cls,
        where: Union[str, pathlib.Path],
        instance_format: str = "vrplib",
        round_func: Union[str, Callable] = no_rounding,
    ) -> "Model":
        """
        Reads a VRPLIB or Solomon formatted file into a model instance.

        .. note::

           This method is a thin wrapper around :func:`~pyvrp.read.read`. See
           that function for details.
        """
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
        vehicle_types = [VehicleType(data.num_vehicles, data.vehicle_capacity)]
        self._vehicle_types = vehicle_types
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
        """
        Adds a client with the given attributes to the model. Returns the
        created :class:`~pyvrp._ProblemData.Client` instance.
        """
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
        """
        Adds a depot with the given attributes to the model. Returns the
        created :class:`~pyvrp._ProblemData.Client` instance.

        .. warning::

           PyVRP does not yet support multi-depot VRPs. For now, only one depot
           can be added to the model.
        """
        if len(self._depots) >= 1:
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
        duration: int = 0,
    ) -> Edge:
        """
        Adds an edge :math:`(i, j)` between ``frm`` (:math:`i`) and ``to``
        (:math:`j`). The edge can be given distance and duration attributes.
        Distance is required, but the default duration is zero. Returns the
        created edge.

        Raises
        ------
        ValueError
            When either distance or duration is a negative value.
        """
        if distance < 0 or duration < 0:
            raise ValueError("Cannot have negative edge distance or duration.")

        edge = Edge(frm, to, distance, duration)
        self._edges.append(edge)
        return edge

    def add_vehicle_type(self, number: int, capacity: int) -> VehicleType:
        """
        Adds a vehicle type with the given number of vehicles of given capacity
        to the model.

        .. warning::

           PyVRP does not yet support heterogeneous fleet VRPs. For now, only
           one vehicle type can be added to the model.

        Raises
        ------
        ValueError
            When either the vehicle number or capacity is not a positive value.
        """
        if len(self._vehicle_types) >= 1:
            msg = "PyVRP does not yet support heterogeneous fleet VRPs."
            raise ValueError(msg)

        if number <= 0:
            raise ValueError("Must have positive number of vehicles.")

        if capacity < 0:
            raise ValueError("Cannot have negative vehicle capacity.")

        vehicle_type = VehicleType(number, capacity)
        self._vehicle_types.append(vehicle_type)
        return vehicle_type

    def update(self):
        """
        Updates the underlying ProblemData instance.
        """
        clients = self._depots + self._clients
        client2idx = {id(client): idx for idx, client in enumerate(clients)}

        num_vehicles = self._vehicle_types[0].number
        vehicle_capacity = self._vehicle_types[0].capacity

        # This should be a sufficiently large maximum value to make sure such
        # edges are never traversed.
        max_value = 10 * max(max(e.distance, e.duration) for e in self._edges)
        if max_value > 0.1 * np.iinfo(np.int32).max:  # >10% of INT_MAX
            msg = """
            The maximum distance or duration value is very large. This might
            impact numerical stability. Consider rescaling your input data.
            """
            warn(msg, ScalingWarning)

        distances = np.full((len(clients), len(clients)), max_value, dtype=int)
        durations = np.full((len(clients), len(clients)), max_value, dtype=int)

        for edge in self._edges:
            frm = client2idx[id(edge.frm)]
            to = client2idx[id(edge.to)]
            distances[frm, to] = edge.distance
            durations[frm, to] = edge.duration

        self._data = ProblemData(
            clients, num_vehicles, vehicle_capacity, distances, durations
        )

    def solve(self, stop: StoppingCriterion, seed: int = 0) -> Result:
        """
        Solve this model.

        Parameters
        ----------
        stop
            Stopping criterion to use.
        seed, optional
            Seed value to use for the PRNG, by default 0.

        Returns
        -------
        Result
            The solution result object, containing the best found solution.
        """
        # These cause a circular import, so the imports needed to be postponed
        # to here (where they are actually used).
        from pyvrp.educate import (
            NODE_OPERATORS,
            ROUTE_OPERATORS,
            LocalSearch,
            compute_neighbours,
        )

        self.update()  # make sure data is available
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
