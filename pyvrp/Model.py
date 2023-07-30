from typing import List, Union
from warnings import warn

import numpy as np

from pyvrp.GeneticAlgorithm import GeneticAlgorithm
from pyvrp.PenaltyManager import PenaltyManager
from pyvrp.Population import Population, PopulationParams
from pyvrp.Result import Result
from pyvrp._pyvrp import (
    Client,
    ProblemData,
    RandomNumberGenerator,
    Solution,
    VehicleType,
)
from pyvrp.constants import MAX_USER_VALUE, MAX_VALUE
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.exceptions import ScalingWarning
from pyvrp.stop import StoppingCriterion

Depot = Client


class Edge:
    """
    Stores an edge connecting two locations.
    """

    __slots__ = ["frm", "to", "distance", "duration"]

    def __init__(self, frm: Client, to: Client, distance: int, duration: int):
        self.frm = frm
        self.to = to
        self.distance = distance
        self.duration = duration


class Model:
    """
    A simple interface for modelling vehicle routing problems with PyVRP.
    """

    def __init__(self) -> None:
        self._clients: List[Client] = []
        self._depots: List[Depot] = []
        self._edges: List[Edge] = []
        self._vehicle_types: List[VehicleType] = []

    @property
    def locations(self) -> List[Client]:
        """
        Returns all locations (depots and clients) in the current model. The
        clients in the routes of the solution returned by :meth:`~solve` can be
        used to index these locations.
        """
        return self._depots + self._clients

    @property
    def vehicle_types(self) -> List[VehicleType]:
        """
        Returns the vehicle types in the current model. The routes of the
        solution returned by :meth:`~solve` have a property
        :meth:`~pyvrp._pyvrp.Route.vehicle_type()` that can be used to index
        these vehicle types.
        """
        return self._vehicle_types

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
        vehicle_types = [
            data.vehicle_type(i) for i in range(data.num_vehicle_types)
        ]
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
        release_time: int = 0,
        prize: int = 0,
        required: bool = True,
    ) -> Client:
        """
        Adds a client with the given attributes to the model. Returns the
        created :class:`~pyvrp._pyvrp.Client` instance.
        """
        client = Client(
            x,
            y,
            demand,
            service_duration,
            tw_early,
            tw_late,
            release_time,
            prize,
            required,
        )

        self._clients.append(client)
        return client

    def add_depot(
        self,
        x: int,
        y: int,
        tw_early: int = 0,
        tw_late: int = 0,
    ) -> Depot:
        """
        Adds a depot with the given attributes to the model. Returns the
        created :class:`~pyvrp._pyvrp.Client` instance.

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

    def add_vehicle_type(
        self, capacity: int, num_available: int
    ) -> VehicleType:
        """
        Adds a vehicle type with the given number of available vehicles of
        given capacity to the model. Returns the created vehicle type.

        Raises
        ------
        ValueError
            When the number of available vehicles or capacity is not a positive
            value.
        """
        if capacity < 0:
            raise ValueError("Cannot have negative vehicle capacity.")

        if num_available <= 0:
            raise ValueError("Must have positive number of vehicles.")

        vehicle_type = VehicleType(capacity, num_available)
        self._vehicle_types.append(vehicle_type)
        return vehicle_type

    def data(self) -> ProblemData:
        """
        Creates and returns a :class:`~pyvrp._pyvrp.ProblemData` instance
        from this model's attributes.
        """
        locs = self.locations
        loc2idx = {id(loc): idx for idx, loc in enumerate(locs)}

        if self._edges:
            max_value = max(max(e.distance, e.duration) for e in self._edges)

            if max_value > MAX_USER_VALUE:
                msg = """
                The maximum distance or duration value is very large. This may
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

        return ProblemData(locs, self.vehicle_types, distances, durations)

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
        from pyvrp.search import (
            NODE_OPERATORS,
            ROUTE_OPERATORS,
            LocalSearch,
            compute_neighbours,
        )

        data = self.data()
        rng = RandomNumberGenerator(seed=seed)
        ls = LocalSearch(data, rng, compute_neighbours(data))

        for node_op in NODE_OPERATORS:
            ls.add_node_operator(node_op(data))

        for route_op in ROUTE_OPERATORS:
            ls.add_route_operator(route_op(data))

        pm = PenaltyManager()
        pop_params = PopulationParams()
        pop = Population(bpd, pop_params)
        init = [
            Solution.make_random(data, rng)
            for _ in range(pop_params.min_pop_size)
        ]

        gen_args = (data, pm, rng, pop, ls, srex, init)
        algo = GeneticAlgorithm(*gen_args)  # type: ignore
        return algo.run(stop)
