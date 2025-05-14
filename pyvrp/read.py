import pathlib
from collections import defaultdict
from itertools import count, pairwise
from numbers import Number
from typing import Callable
from warnings import warn

import numpy as np
import vrplib

from pyvrp._pyvrp import (
    Client,
    ClientGroup,
    Depot,
    ProblemData,
    Route,
    Solution,
    Trip,
    VehicleType,
)
from pyvrp.constants import MAX_VALUE
from pyvrp.exceptions import ScalingWarning

_RoundingFunc = Callable[[np.ndarray], np.ndarray]

_INT_MAX = np.iinfo(np.int64).max
_UINT_MAX = np.iinfo(np.uint64).max


ROUND_FUNCS: dict[str, _RoundingFunc] = {
    "round": lambda vals: np.round(vals).astype(np.int64),
    "trunc": lambda vals: vals.astype(np.int64),
    "dimacs": lambda vals: (10 * vals).astype(np.int64),
    "exact": lambda vals: np.round(1_000 * vals).astype(np.int64),
    "none": lambda vals: vals,
}


def read(
    where: str | pathlib.Path,
    round_func: str | _RoundingFunc = "none",
) -> ProblemData:
    """
    Reads the ``VRPLIB`` file at the given location, and returns a
    :class:`~pyvrp._pyvrp.ProblemData` instance.

    .. note::

       See the
       :doc:`VRPLIB format explanation <../dev/supported_vrplib_fields>` page
       for more details.

    Parameters
    ----------
    where
        File location to read. Assumes the data on the given location is in
        ``VRPLIB`` format.
    round_func
        Optional rounding function that is applied to all data values in the
        instance. This can either be a function or a string:

            * ``'round'`` rounds the values to the nearest integer;
            * ``'trunc'`` truncates the values to an integer;
            * ``'dimacs'`` scales by 10 and truncates the values to an integer;
            * ``'exact'`` scales by 1000 and rounds to the nearest integer.
            * ``'none'`` does no rounding. This is the default.

    Raises
    ------
    TypeError
        When ``round_func`` does not name a rounding function, or is not
        callable.
    ValueError
        When the data file does not provide information on the problem size.

    Returns
    -------
    ProblemData
        Data instance constructed from the read data.
    """
    if (key := str(round_func)) in ROUND_FUNCS:
        round_func = ROUND_FUNCS[key]

    if not callable(round_func):
        raise TypeError(
            f"round_func = {round_func} is not understood. Can be a function,"
            f" or one of {ROUND_FUNCS.keys()}."
        )

    parser = _InstanceParser(vrplib.read_instance(where), round_func)
    builder = _ProblemDataBuilder(parser)
    return builder.data()


def read_solution(where: str | pathlib.Path, data: ProblemData) -> Solution:
    """
    Reads a solution in ``VRPLIB`` format from the give file location, and
    returns the corresponding Solution object.

    Parameters
    ----------
    where
        File location to read. Assumes the solution in the file on the given
        location is in ``VRPLIB`` solution format.
    data
        Problem data instance that the solution is based on. See
        :meth:`~pyvrp.read` for details.

    Returns
    -------
    Solution
        Solution object constructed from the read data.
    """
    sol = vrplib.read_solution(str(where))

    # We assume that the routes are listed in order of vehicle types as
    # determined by ``read()``. We particularly rely on the indices ``read()``
    # encodes in the vehicle type's name to map between vehicles and types.
    veh2type = np.zeros((data.num_vehicles,), dtype=int)
    for idx, veh_type in enumerate(data.vehicle_types()):
        idcs = list(map(int, veh_type.name.split(",")))
        veh2type[idcs] = idx

    routes = []
    for idx, route in enumerate(sol["routes"]):
        if not route:
            continue

        route_visits = np.array(route, dtype=int)
        depot_idcs = np.flatnonzero(route_visits < data.num_depots)

        trip_visits = np.split(route_visits, depot_idcs)
        trip_visits = [
            # These visits include the reload depots for later trips as the
            # first trip visit, which we need to skip.
            trip_visits[trip_idx > 0 :]
            for trip_idx, trip_visits in enumerate(trip_visits)
        ]

        veh_type = data.vehicle_type(veh2type[idx])
        depots = [
            veh_type.start_depot,
            *route_visits[depot_idcs],
            veh_type.end_depot,
        ]

        trips = [
            Trip(data, visits, veh2type[idx], start, end)
            for visits, (start, end) in zip(trip_visits, pairwise(depots))
        ]

        routes.append(Route(data, trips, veh2type[idx]))

    return Solution(data, routes)


class _InstanceParser:
    """
    read() helper that parses VRPLIB data into meaningful parts for further
    processing.
    """

    def __init__(self, instance: dict, round_func: _RoundingFunc):
        self.instance = instance
        self.round_func = round_func

    @property
    def num_locations(self) -> int:
        return self.instance["dimension"]

    @property
    def num_depots(self) -> int:
        return self.instance.get("depot", np.array([0])).size

    @property
    def num_clients(self) -> int:
        return self.num_locations - self.num_depots

    @property
    def num_vehicles(self) -> int:
        return self.instance.get("vehicles", self.num_locations - 1)

    def type(self) -> str:
        return self.instance.get("type", "")

    def edge_weight(self) -> np.ndarray:
        return self.round_func(self.instance["edge_weight"])

    def depot_idcs(self) -> np.ndarray:
        return self.instance.get("depot", np.array([0]))

    def backhauls(self) -> np.ndarray:
        if "backhaul" not in self.instance:
            return np.zeros((self.num_locations, 1), dtype=np.int64)

        return self.round_func(self.instance["backhaul"])

    def demands(self) -> np.ndarray:
        if "demand" not in self.instance and "linehaul" not in self.instance:
            return np.zeros((self.num_locations, 1), dtype=np.int64)

        return self.round_func(
            self.instance.get("demand", self.instance.get("linehaul"))
        )

    def coords(self) -> np.ndarray:
        if "node_coord" not in self.instance:
            return np.zeros((self.num_locations, 2), dtype=np.int64)

        return self.round_func(self.instance["node_coord"])

    def service_times(self) -> np.ndarray:
        service_times = self.instance.get("service_time", 0)

        if isinstance(service_times, Number):
            # Some instances describe a uniform service time as a single value
            # that applies to all clients.
            service_times = np.full(self.num_locations, service_times)
            service_times[: self.num_depots] = 0

        return self.round_func(service_times)

    def time_windows(self) -> np.ndarray:
        if "time_window" not in self.instance:
            time_windows = np.empty((self.num_locations, 2), dtype=np.int64)
            time_windows[:, 0] = 0
            time_windows[:, 1] = _INT_MAX
            return time_windows

        return self.round_func(self.instance["time_window"])

    def release_times(self) -> np.ndarray:
        release_times = self.instance.get("release_time", 0)
        shape = self.num_locations
        return self.round_func(np.broadcast_to(release_times, shape))

    def reload_depots(self) -> list[tuple[int, ...]]:
        if "vehicles_reload_depot" not in self.instance:
            return [tuple() for _ in range(self.num_vehicles)]

        reload_depots = self.instance["vehicles_reload_depot"]

        if isinstance(reload_depots[0], Number):
            # Some instances describe only one reload depot per vehicle, so
            # we first cast it to a 2D array.
            reload_depots = np.atleast_2d(reload_depots).T

        return [tuple(idx - 1 for idx in depots) for depots in reload_depots]

    def prizes(self) -> np.ndarray:
        if "prize" not in self.instance:
            return np.zeros(self.num_locations, dtype=np.int64)

        return self.round_func(self.instance["prize"])

    def capacities(self) -> np.ndarray:
        if "capacity" not in self.instance:
            return np.full(self.num_vehicles, _INT_MAX)

        capacities = self.instance["capacity"]

        if isinstance(capacities, Number):
            # Some instances describe a uniform capacity as a single value
            # that applies to all vehicles.
            capacities = np.full(self.num_vehicles, capacities)

        return self.round_func(capacities)

    def allowed_clients(self) -> list[tuple[int, ...]]:
        if "vehicles_allowed_clients" not in self.instance:
            client_idcs = tuple(range(self.num_depots, self.num_locations))
            return [client_idcs for _ in range(self.num_vehicles)]

        allowed_clients = self.instance["vehicles_allowed_clients"]

        if isinstance(allowed_clients[0], Number):
            # Some instances describe only one allowed client per vehicle, so
            # we first cast it to a 2D array.
            allowed_clients = np.atleast_2d(allowed_clients).T

        return [
            tuple(idx - 1 for idx in clients) for clients in allowed_clients
        ]

    def vehicles_depots(self) -> np.ndarray:
        if "vehicles_depot" not in self.instance:
            depot_idcs = self.depot_idcs()
            return np.full(self.num_vehicles, depot_idcs[0])

        return self.instance["vehicles_depot"] - 1  # zero-indexed

    def max_distances(self) -> np.ndarray:
        if "vehicles_max_distance" not in self.instance:
            return np.full(self.num_vehicles, _INT_MAX)

        max_distances = self.instance["vehicles_max_distance"]
        shape = self.num_vehicles
        return self.round_func(np.broadcast_to(max_distances, shape))

    def max_durations(self) -> np.ndarray:
        if "vehicles_max_duration" not in self.instance:
            return np.full(self.num_vehicles, _INT_MAX)

        max_durations = self.instance["vehicles_max_duration"]
        shape = self.num_vehicles
        return self.round_func(np.broadcast_to(max_durations, shape))

    def max_reloads(self) -> np.ndarray:
        max_reloads = self.instance.get("vehicles_max_reloads", _UINT_MAX)
        return np.broadcast_to(max_reloads, self.num_vehicles)

    def fixed_costs(self) -> np.ndarray:
        fixed_costs = self.instance.get("vehicles_fixed_cost", 0)
        return self.round_func(np.broadcast_to(fixed_costs, self.num_vehicles))

    def unit_distance_costs(self) -> np.ndarray:
        # Unit distance costs are unrounded to prevent double scaling in the
        # total distance cost calculation (unit_distance_cost * distance).
        unit_cost = self.instance.get("vehicles_unit_distance_cost", 1)
        return np.broadcast_to(unit_cost, self.num_vehicles)

    def mutually_exclusive_groups(self) -> list[list[int]]:
        if "mutually_exclusive_group" not in self.instance:
            return []

        groups = self.instance["mutually_exclusive_group"]

        if isinstance(groups[0], Number):
            # Some instances describe only one client per group, so we first
            # cast it to a 2D array.
            groups = np.atleast_2d(groups).T

        raw_groups = [[idx - 1 for idx in group] for group in groups]

        # Only keep groups if they have more than one member. Empty groups or
        # groups with one member are trivial to decide, so there is no point
        # in keeping them.
        return [group for group in raw_groups if len(group) > 1]


class _ProblemDataBuilder:
    """
    read() helper that builds a ``ProblemData`` object from the instance
    attributes of the given parser.
    """

    def __init__(self, parser: _InstanceParser):
        self.parser = parser

    def data(self) -> ProblemData:
        clients = self._clients()
        depots = self._depots()
        vehicle_types = self._vehicle_types()
        distance_matrices = self._distance_matrices()
        groups = self._groups()

        return ProblemData(
            clients=clients,
            depots=depots,
            vehicle_types=vehicle_types,
            distance_matrices=distance_matrices,
            # VRPLIB instances typically do not have a duration data field, and
            # instead assume duration == distance.
            duration_matrices=distance_matrices,
            groups=groups,
        )

    def _depots(self) -> list[Depot]:
        num_depots = self.parser.num_depots
        depot_idcs = self.parser.depot_idcs()

        contiguous_lower_idcs = np.arange(num_depots)
        if num_depots == 0 or (depot_idcs != contiguous_lower_idcs).any():
            msg = """
            Source file should contain at least one depot in the contiguous
            lower indices, starting from 1.
            """
            raise ValueError(msg)

        coords = self.parser.coords()
        return [
            Depot(x=coords[idx][0], y=coords[idx][1])
            for idx in range(num_depots)
        ]

    def _clients(self) -> list[Client]:
        groups = self.parser.mutually_exclusive_groups()
        num_locs = self.parser.num_locations

        idx2group: list[int | None] = [None for _ in range(num_locs)]
        for group, members in enumerate(groups):
            for client in members:
                idx2group[client] = group

        coords = self.parser.coords()
        demands = self.parser.demands()
        backhauls = self.parser.backhauls()
        service_duration = self.parser.service_times()
        time_windows = self.parser.time_windows()
        release_times = self.parser.release_times()
        prizes = self.parser.prizes()  # we interpret a zero-prize client as
        required = np.isclose(prizes, 0)  # required in the benchmark instances

        return [
            Client(
                x=coords[idx][0],
                y=coords[idx][1],
                delivery=np.atleast_1d(demands[idx]),
                pickup=np.atleast_1d(backhauls[idx]),
                service_duration=service_duration[idx],
                tw_early=time_windows[idx][0],
                tw_late=time_windows[idx][1],
                release_time=release_times[idx],
                prize=prizes[idx],
                required=required[idx] and idx2group[idx] is None,
                group=idx2group[idx],
            )
            for idx in range(self.parser.num_depots, num_locs)
        ]

    def _vehicle_types(self) -> list[VehicleType]:
        num_vehicles = self.parser.num_vehicles
        vehicles_data = (
            self.parser.capacities(),
            self.parser.allowed_clients(),
            self.parser.reload_depots(),
            self.parser.vehicles_depots(),
            self.parser.max_distances(),
            self.parser.max_durations(),
            self.parser.max_reloads(),
            self.parser.fixed_costs(),
            self.parser.unit_distance_costs(),
        )

        if any(len(attr) != num_vehicles for attr in vehicles_data):
            msg = """
            The number of elements in the vehicles data attributes should be
            equal to the number of vehicles in the problem.
            """
            raise ValueError(msg)

        # VRPLIB instances includes data for each available vehicle. We group
        # vehicles by their attributes to create unique vehicle types.
        type2idcs = defaultdict(list)
        for vehicle, (capacity, *veh_type) in enumerate(zip(*vehicles_data)):
            capacity = tuple(np.atleast_1d(capacity))
            type2idcs[(capacity, *veh_type)].append(vehicle)

        client2profile = self._allowed2profile()
        time_windows = self.parser.time_windows()

        vehicle_types = []
        for attributes, vehicles in type2idcs.items():
            (
                capacity,
                clients,
                reloads,
                depot,
                max_distance,
                max_duration,
                max_reloads,
                fixed_cost,
                unit_distance_cost,
            ) = attributes

            vehicle_type = VehicleType(
                num_available=len(vehicles),
                capacity=capacity,
                start_depot=depot,
                end_depot=depot,
                fixed_cost=fixed_cost,
                # The literature specifies depot time windows. We do not have
                # depot time windows but instead set those on the vehicles.
                tw_early=time_windows[depot][0],
                tw_late=time_windows[depot][1],
                max_duration=max_duration,
                max_distance=max_distance,
                unit_distance_cost=unit_distance_cost,
                profile=client2profile[clients],
                reload_depots=reloads,
                max_reloads=max_reloads,
                # A bit hacky, but this csv-like name is really useful to track
                # the actual vehicles that make up this vehicle type.
                name=",".join(map(str, vehicles)),
            )
            vehicle_types.append(vehicle_type)

        return vehicle_types

    def _distance_matrices(self) -> list[np.ndarray]:
        distances = self.parser.edge_weight()

        if self.parser.type() == "VRPB":
            # In VRPB, linehauls must be served before backhauls. This can be
            # enforced by setting a high value for the distance/duration from
            # depot to backhaul (forcing linehaul to be served first) and a
            # large value from backhaul to linehaul (avoiding linehaul after
            # backhaul clients).
            linehaul = self.parser.demands() > 0
            backhaul = self.parser.backhauls() > 0
            distances[0, backhaul] = MAX_VALUE
            distances[np.ix_(backhaul, linehaul)] = MAX_VALUE

        allowed2profile = self._allowed2profile()
        num_profiles = len(allowed2profile)
        dist_mats = [distances.copy() for _ in range(num_profiles)]

        for allowed_clients, type_idx in allowed2profile.items():
            if len(allowed_clients) == self.parser.num_clients:
                # True if this feature is unused, and the distance matrix for
                # this profile does not have to be modified.
                continue

            num_depots = self.parser.num_depots
            num_locations = self.parser.num_locations

            # This profile is allowed to visit every depot and all clients in
            # its allowed clients section.
            allowed = np.zeros((num_locations,), dtype=bool)
            allowed[:num_depots] = True
            allowed[list(allowed_clients)] = True

            # Some dtype trickery to ensure the MAX_VALUE assignment below does
            # not overflow.
            dtype = np.promote_types(dist_mats[type_idx].dtype, np.int64)
            dist_mats[type_idx] = dist_mats[type_idx].astype(dtype)

            # Set MAX_VALUE to and from disallowed clients, preventing this
            # vehicle type from serving them.
            dist_mat = dist_mats[type_idx]
            dist_mat[:, ~allowed] = dist_mat[~allowed, :] = MAX_VALUE
            np.fill_diagonal(dist_mat, 0)

        if any(dist.max() > MAX_VALUE for dist in dist_mats):
            msg = """
            The maximum distance value is very large. This might impact 
            numerical stability. Consider rescaling your input data.
            """
            warn(msg, ScalingWarning)

        return dist_mats

    def _groups(self) -> list[ClientGroup]:
        groups = self.parser.mutually_exclusive_groups()
        return [ClientGroup(group) for group in groups]

    def _allowed2profile(self) -> dict[tuple[int, ...], int]:
        allowed_clients2profile_idx = {}
        profile_idx = count(0)
        for clients in self.parser.allowed_clients():
            if clients not in allowed_clients2profile_idx:
                allowed_clients2profile_idx[clients] = next(profile_idx)

        return allowed_clients2profile_idx
