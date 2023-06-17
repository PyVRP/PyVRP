from typing import List, Union
from warnings import warn

import numpy as np

from pyvrp.GeneticAlgorithm import GeneticAlgorithm, GeneticAlgorithmParams
from pyvrp.PenaltyManager import PenaltyManager
from pyvrp.Population import Population, PopulationParams
from pyvrp.Result import Result
from pyvrp._ProblemData import Client, ProblemData
from pyvrp._Solution import Solution
from pyvrp._XorShift128 import XorShift128
from pyvrp.constants import MAX_USER_VALUE, MAX_VALUE
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.exceptions import ScalingWarning
from pyvrp.stop import StoppingCriterion

Depot = Client


class Edge:

    __slots__ = ["frm", "to", "distance", "duration"]

    def __init__(self, frm: Client, to: Client, distance: int, duration: int):
        self.frm = frm
        self.to = to
        self.distance = distance
        self.duration = duration


class VehicleType:

    __slots__ = ["number", "capacity"]

    def __init__(self, number: int, capacity: int):
        self.number = number
        self.capacity = capacity


class Model:
    """
    A simple interface for modelling vehicle routing problems with PyVRP.
    """

    def __init__(self):
        self._clients: List[Client] = []
        self._depots: List[Depot] = []
        self._edges: List[Edge] = []
        self._vehicle_types: List[VehicleType] = []

    @property
    def locations(self) -> List[Client]:
        """
        Returns all locations (depots and clients) in the current model. The
        routes returned by :meth:`~solve` can be used to index these locations.
        """
        return self._depots + self._clients

    @classmethod
    def from_data(cls, data: ProblemData) -> "Model":
        """
        Constructs a model instance from the given data.

        Parameters
        ----------
        data
            Problem data to feed into the model.

        Returns
        -------
        Model
            A model instance representing the given data.
        """
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
        to the model. Returns the created vehicle type.

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

    def data(self) -> ProblemData:
        """
        Creates and returns a :class:`~pyvrp._ProblemData.ProblemData` instance
        from this model's attributes.
        """
        locs = self.locations
        loc2idx = {id(loc): idx for idx, loc in enumerate(locs)}

        num_vehicles = self._vehicle_types[0].number
        vehicle_capacity = self._vehicle_types[0].capacity

        max_data_value = max(max(e.distance, e.duration) for e in self._edges)
        if max_data_value > MAX_USER_VALUE:
            msg = """
            The maximum distance or duration value is very large. This might
            impact numerical stability. Consider rescaling your input data.
            """
            warn(msg, ScalingWarning)

        # Default value is a sufficiently large value to make sure any edges
        # not set below are never traversed.
        distances = np.full((len(locs), len(locs)), MAX_VALUE, dtype=int)
        durations = np.full((len(locs), len(locs)), MAX_VALUE, dtype=int)

        for edge in self._edges:
            frm = loc2idx[id(edge.frm)]
            to = loc2idx[id(edge.to)]
            distances[frm, to] = edge.distance
            durations[frm, to] = edge.duration

        return ProblemData(
            locs,
            num_vehicles,
            vehicle_capacity,
            distances,
            durations,
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

        data = self.data()
        rng = XorShift128(seed=seed)
        ls = LocalSearch(data, rng, compute_neighbours(data))

        for op in NODE_OPERATORS:
            ls.add_node_operator(op(data))

        for op in ROUTE_OPERATORS:
            ls.add_route_operator(op(data))

        pm = PenaltyManager()
        pop_params = PopulationParams()
        pop = Population(bpd, pop_params)
        init = [
            Solution.make_random(data, rng)
            for _ in range(pop_params.min_pop_size)
        ]

        gen_params = GeneticAlgorithmParams(collect_statistics=True)
        algo = GeneticAlgorithm(data, pm, rng, pop, ls, srex, init, gen_params)
        return algo.run(stop)
