from typing import Optional, Union
from warnings import warn

import numpy as np

from pyvrp.Config import Config
from pyvrp.GeneticAlgorithm import GeneticAlgorithm
from pyvrp.PenaltyManager import PenaltyManager
from pyvrp.Population import Population
from pyvrp.Result import Result
from pyvrp._pyvrp import (
    Client,
    Depot,
    ProblemData,
    RandomNumberGenerator,
    Solution,
    VehicleType,
)
from pyvrp.constants import MAX_VALUE
from pyvrp.crossover import ordered_crossover as ox
from pyvrp.crossover import selective_route_exchange as srex
from pyvrp.diversity import broken_pairs_distance as bpd
from pyvrp.exceptions import ScalingWarning
from pyvrp.search import (
    LocalSearch,
    compute_neighbours,
)
from pyvrp.stop import StoppingCriterion


class Edge:
    """
    Stores an edge connecting two locations.
    """

    __slots__ = ["frm", "to", "distance", "duration"]

    def __init__(
        self,
        frm: Union[Client, Depot],
        to: Union[Client, Depot],
        distance: int,
        duration: int,
    ):
        self.frm = frm
        self.to = to
        self.distance = distance
        self.duration = duration


class Model:
    """
    A simple interface for modelling vehicle routing problems with PyVRP.
    """

    def __init__(self) -> None:
        self._clients: list[Client] = []
        self._depots: list[Depot] = []
        self._edges: list[Edge] = []
        self._vehicle_types: list[VehicleType] = []

    @property
    def locations(self) -> list[Union[Client, Depot]]:
        """
        Returns all locations (depots and clients) in the current model. The
        clients in the routes of the solution returned by :meth:`~solve` can be
        used to index these locations.
        """
        return self._depots + self._clients

    @property
    def vehicle_types(self) -> list[VehicleType]:
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
        depots = data.depots()
        clients = data.clients()
        locs = depots + clients
        edges = [
            Edge(
                locs[frm],
                locs[to],
                data.dist(frm, to),
                data.duration(frm, to),
            )
            for frm in range(data.num_locations)
            for to in range(data.num_locations)
        ]

        self = Model()
        self._clients = clients
        self._depots = depots
        self._edges = edges
        self._vehicle_types = data.vehicle_types()

        return self

    def add_client(
        self,
        x: int,
        y: int,
        delivery: int = 0,
        pickup: int = 0,
        service_duration: int = 0,
        tw_early: int = 0,
        tw_late: int = np.iinfo(np.int64).max,
        release_time: int = 0,
        prize: int = 0,
        required: bool = True,
        name: str = "",
    ) -> Client:
        """
        Adds a client with the given attributes to the model. Returns the
        created :class:`~pyvrp._pyvrp.Client` instance.
        """
        client = Client(
            x,
            y,
            delivery,
            pickup,
            service_duration,
            tw_early,
            tw_late,
            release_time,
            prize,
            required,
            name,
        )

        self._clients.append(client)
        return client

    def add_depot(
        self,
        x: int,
        y: int,
        tw_early: int = 0,
        tw_late: int = np.iinfo(np.int64).max,
        name: str = "",
    ) -> Depot:
        """
        Adds a depot with the given attributes to the model. Returns the
        created :class:`~pyvrp._pyvrp.Depot` instance.
        """
        depot = Depot(x, y, tw_early=tw_early, tw_late=tw_late, name=name)
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
            When either distance or duration is a negative value, or when self
            loops have nonzero distance or duration values.
        """
        if distance < 0 or duration < 0:
            raise ValueError("Cannot have negative edge distance or duration.")

        if frm == to and (distance != 0 or duration != 0):
            raise ValueError("A self loop must have 0 distance and duration.")

        if max(distance, duration) > MAX_VALUE:
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
        num_available: int = 1,
        capacity: int = 0,
        depot: Optional[Depot] = None,
        fixed_cost: int = 0,
        tw_early: int = 0,
        tw_late: int = np.iinfo(np.int64).max,
        max_duration: int = np.iinfo(np.int64).max,
        max_distance: int = np.iinfo(np.int64).max,
        name: str = "",
    ) -> VehicleType:
        """
        Adds a vehicle type with the given attributes to the model. Returns the
        created vehicle type.

        .. note::

           The vehicle type is assigned to the first depot if ``depot`` is not
           provided.

        Raises
        ------
        ValueError
            When the given ``depot`` is not already added to this model
            instance.
        """
        if depot is None:
            depot_idx = 0
        elif depot in self._depots:
            depot_idx = self._depots.index(depot)
        else:
            raise ValueError("The given depot is not in this model instance.")

        vehicle_type = VehicleType(
            num_available,
            capacity,
            depot_idx,
            fixed_cost,
            tw_early,
            tw_late,
            max_duration,
            max_distance,
            name,
        )

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
        np.fill_diagonal(distances, 0)
        np.fill_diagonal(durations, 0)

        for edge in self._edges:
            frm = loc2idx[id(edge.frm)]
            to = loc2idx[id(edge.to)]
            distances[frm, to] = edge.distance
            durations[frm, to] = edge.duration

        return ProblemData(
            self._clients,
            self._depots,
            self.vehicle_types,
            distances,
            durations,
        )

    def solve(
        self,
        stop: StoppingCriterion,
        config: Config = Config(),
        seed: int = 0,
        display: bool = True,
    ) -> Result:
        """
        Solve this model.

        Parameters
        ----------
        stop
            Stopping criterion to use.
        config
            Configuration to use.
        seed
            Seed value to use for the random number stream. Default 0.
        display
            Whether to display information about the solver progress. Default
            ``True``.

        Returns
        -------
        Result
            The solution result object, containing the best found solution.
        """
        data = self.data()
        rng = RandomNumberGenerator(seed=seed)

        neighbours = compute_neighbours(data, config.neighbourhood)
        ls = LocalSearch(data, rng, neighbours)

        for node_op in config.node_ops:
            ls.add_node_operator(node_op(data))

        for route_op in config.route_ops:
            ls.add_route_operator(route_op(data))

        pm = PenaltyManager(config.penalty)
        pop = Population(bpd, config.population)
        init = [
            Solution.make_random(data, rng)
            for _ in range(config.population.min_pop_size)
        ]

        # We use SREX when the instance is a proper VRP; else OX for TSP.
        crossover = srex if data.num_vehicles > 1 else ox

        gen_args = (data, pm, rng, pop, ls, crossover, init, config.genetic)
        algo = GeneticAlgorithm(*gen_args)  # type: ignore
        return algo.run(stop, display)
