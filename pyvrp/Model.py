from typing import List, Union
from warnings import warn

import numpy as np

from pyvrp.GeneticAlgorithm import GeneticAlgorithm
from pyvrp.PenaltyManager import PenaltyManager
from pyvrp.Population import Population, PopulationParams
from pyvrp.Result import Result
from pyvrp._pyvrp import Break as _Break
from pyvrp._pyvrp import (
    Client,
    ProblemData,
    RandomNumberGenerator,
    Solution,
)
from pyvrp._pyvrp import VehicleType as _VehicleType
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


class Break:
    """
    Wrapper around the native Break, because that takes indices, not
    location objects, and the indices are not known yet during model
    construction.
    """

    def __init__(
        self,
        location: Client,
        duration: int = 0,
        tw_early: int = 0,
        tw_late: int = 0,
    ):
        self.location = location
        self.duration = duration
        self.tw_early = tw_early
        self.tw_late = tw_late


class VehicleType:
    """
    Wrapper around the native VehicleType, because that takes indices, not
    location objects, and the indices are not known yet during model
    construction.
    """

    def __init__(self, capacity: int, num_available: int, fixed_cost: int = 0):
        self.capacity = capacity
        self.num_available = num_available
        self.fixed_cost = fixed_cost
        self.breaks: List[Break] = []

    def add_break(
        self,
        location: Client,
        duration: int = 0,
        tw_early: int = 0,
        tw_late: int = 0,
    ) -> Break:
        """
        Adds a break to this vehicle type.
        """
        brk = Break(location, duration, tw_early, tw_late)
        self.breaks.append(brk)
        return brk


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

        self._vehicle_types = []
        for idx in range(data.num_vehicle_types):
            native_type = data.vehicle_type(idx)
            vehicle_type = VehicleType(
                native_type.capacity,
                native_type.num_available,
                native_type.fixed_cost,
            )

            for brk in native_type.breaks:
                vehicle_type.add_break(
                    clients[brk.location],
                    brk.duration,
                    brk.tw_early,
                    brk.tw_late,
                )

            self.vehicle_types.append(vehicle_type)

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

        if max(distance, duration) > MAX_USER_VALUE:
            msg = """
            The given distance or duration value is very large. This may impact
            numerical stability. Consider rescaling your input data.
            """
            warn(msg, ScalingWarning)

        edge = Edge(frm, to, distance, duration)
        self._edges.append(edge)
        return edge

    def add_vehicle_type(
        self,
        capacity: int,
        num_available: int,
        fixed_cost: int = 0,
    ) -> VehicleType:
        """
        Adds a vehicle type with the given attributes to the model. Returns the
        created vehicle type.
        """
        vehicle_type = VehicleType(capacity, num_available, fixed_cost)
        self._vehicle_types.append(vehicle_type)
        return vehicle_type

    def data(self) -> ProblemData:
        """
        Creates and returns a :class:`~pyvrp._pyvrp.ProblemData` instance
        from this model's attributes.
        """
        locs = self.locations
        loc2idx = {id(loc): idx for idx, loc in enumerate(locs)}

        # Default value is a sufficiently large value to make sure any edges
        # not set below are never traversed.
        distances = np.full((len(locs), len(locs)), MAX_VALUE, dtype=int)
        durations = np.full((len(locs), len(locs)), MAX_VALUE, dtype=int)

        for edge in self._edges:
            frm = loc2idx[id(edge.frm)]
            to = loc2idx[id(edge.to)]
            distances[frm, to] = edge.distance
            durations[frm, to] = edge.duration

        vehicle_types = []
        for vehicle_type in self._vehicle_types:
            breaks = [
                _Break(
                    loc2idx[id(brk.location)],
                    brk.duration,
                    brk.tw_early,
                    brk.tw_late,
                )
                for brk in vehicle_type.breaks
            ]

            vehicle_types.append(
                _VehicleType(
                    vehicle_type.capacity,
                    vehicle_type.num_available,
                    vehicle_type.fixed_cost,
                    breaks,
                )
            )

        return ProblemData(locs, vehicle_types, distances, durations)

    def solve(self, stop: StoppingCriterion, seed: int = 0) -> Result:
        """
        Solve this model.

        Parameters
        ----------
        stop
            Stopping criterion to use.
        seed
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
