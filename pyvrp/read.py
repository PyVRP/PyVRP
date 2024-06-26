import pathlib
from collections import defaultdict
from functools import cached_property
from numbers import Number
from typing import Callable, Optional, Union
from warnings import warn

import numpy as np
import vrplib

from pyvrp._pyvrp import Client, ClientGroup, Depot, ProblemData, VehicleType
from pyvrp.constants import MAX_VALUE
from pyvrp.exceptions import ScalingWarning

_Routes = list[list[int]]
_RoundingFunc = Callable[[np.ndarray], np.ndarray]

_INT_MAX = np.iinfo(np.int64).max


def round_nearest(vals: np.ndarray):
    return np.round(vals).astype(np.int64)


def truncate(vals: np.ndarray):
    return vals.astype(np.int64)


def dimacs(vals: np.ndarray):
    return (10 * vals).astype(np.int64)


def exact(vals: np.ndarray):
    return round_nearest(1_000 * vals)


def no_rounding(vals):
    return vals


ROUND_FUNCS: dict[str, _RoundingFunc] = {
    "round": round_nearest,
    "trunc": truncate,
    "dimacs": dimacs,
    "exact": exact,
    "none": no_rounding,
}


def read(
    where: Union[str, pathlib.Path],
    round_func: Union[str, _RoundingFunc] = "none",
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
    instance = Instance(vrplib.read_instance(where), round_func)

    clients = _clients(instance)
    depots = _depots(instance)
    vehicle_types = _vehicle_types(instance)
    distance_matrices, duration_matrices = _matrices(instance, vehicle_types)
    groups = _groups(instance)

    return ProblemData(
        clients=clients,
        depots=depots,
        vehicle_types=vehicle_types,
        distance_matrices=distance_matrices,
        duration_matrices=duration_matrices,
        groups=groups,
    )


def read_solution(where: Union[str, pathlib.Path]) -> _Routes:
    """
    Reads a solution in ``VRPLIB`` format from the give file location, and
    returns the routes contained in it.

    Parameters
    ----------
    where
        File location to read. Assumes the solution in the file on the given
        location is in ``VRPLIB`` solution format.

    Returns
    -------
    list
        List of routes, where each route is a list of client numbers.
    """
    sol = vrplib.read_solution(str(where))
    return sol["routes"]  # type: ignore


class Instance:
    """
    Helper class for VRPLIB instance data. It sets default values and applies
    rounding function to the data when necessary.

    Parameters
    ----------
    instance
        The original VRPLIB instance data.
    round_func
        The rounding function to apply to the data.
    """

    def __init__(self, instance: dict, round_func: Union[str, _RoundingFunc]):
        if (key := str(round_func)) in ROUND_FUNCS:
            round_func = ROUND_FUNCS[key]

        if not callable(round_func):
            raise TypeError(
                f"round_func = {round_func} is not understood. Can be a"
                f" function, or one of {ROUND_FUNCS.keys()}."
            )

        self._instance = instance
        self._round_func = round_func

    @cached_property
    def dimension(self) -> int:
        return self._instance.get(
            "dimension", self._instance["edge_weight"].shape[0]
        )

    @cached_property
    def num_depots(self) -> int:
        return self._instance.get("depot", np.array([0])).size

    @cached_property
    def num_vehicles(self) -> int:
        return self._instance.get("vehicles", self.dimension - 1)

    @cached_property
    def type(self) -> str:
        return self._instance.get("type", "")

    @cached_property
    def edge_weight(self) -> np.ndarray:
        return self._round_func(self._instance["edge_weight"])

    @cached_property
    def backhauls(self) -> np.ndarray:
        if "backhaul" not in self._instance:
            return np.zeros(self.dimension, dtype=np.int64)

        return self._round_func(self._instance["backhaul"])

    @cached_property
    def demands(self) -> np.ndarray:
        if "demand" not in self._instance and "linehaul" not in self._instance:
            return np.zeros(self.dimension, dtype=np.int64)

        return self._round_func(
            self._instance.get("demand", self._instance.get("linehaul"))
        )

    @cached_property
    def coords(self) -> np.ndarray:
        if "node_coord" not in self._instance:
            return np.zeros((self.dimension, 2), dtype=np.int64)

        return self._round_func(self._instance["node_coord"])

    @cached_property
    def service_times(self) -> np.ndarray:
        if "service_time" not in self._instance:
            return np.zeros(self.dimension, dtype=np.int64)

        service_times = self._instance["service_time"]

        if isinstance(service_times, Number):
            # Some instances describe a uniform service time as a single value
            # that applies to all clients.
            service_times = np.full(self.dimension, service_times)
            service_times[0] = 0

        return self._round_func(service_times)

    @cached_property
    def time_windows(self) -> np.ndarray:
        if "time_window" not in self._instance:
            time_windows = np.empty((self.dimension, 2), dtype=np.int64)
            time_windows[:, 0] = 0
            time_windows[:, 1] = _INT_MAX
            return time_windows

        return self._round_func(self._instance["time_window"])

    @cached_property
    def release_times(self) -> np.ndarray:
        if "release_time" not in self._instance:
            return np.zeros(self.dimension, dtype=np.int64)

        return self._round_func(self._instance["release_time"])

    @cached_property
    def prizes(self) -> np.ndarray:
        if "prize" not in self._instance:
            return np.zeros(self.dimension, dtype=np.int64)

        return self._round_func(self._instance["prize"])

    @cached_property
    def depot_idcs(self) -> np.ndarray:
        return self._instance.get("depot", np.array([0]))

    @cached_property
    def capacities(self) -> np.ndarray:
        if "capacity" not in self._instance:
            return np.full(self.num_vehicles, _INT_MAX)

        capacities = self._instance["capacity"]

        if isinstance(capacities, Number):
            # Some instances describe a uniform capacity as a single value
            # that applies to all vehicles.
            capacities = np.full(self.num_vehicles, capacities)

        return self._round_func(capacities)

    @cached_property
    def vehicles_allowed_clients(self) -> list[tuple[int, ...]]:
        if "vehicles_allowed_clients" not in self._instance:
            client_idcs = tuple(range(self.num_depots, self.dimension))
            return [client_idcs for _ in range(self.num_vehicles)]

        return [
            tuple(idx - 1 for idx in clients)
            for clients in self._instance["vehicles_allowed_clients"]
        ]

    @cached_property
    def vehicles_depots(self) -> np.ndarray:
        if "vehicles_depot" not in self._instance:
            return np.full(self.num_vehicles, self.depot_idcs[0])

        return self._instance["vehicles_depot"] - 1  # zero-indexed

    @cached_property
    def vehicles_max_distance(self) -> np.ndarray:
        if "vehicles_max_distance" not in self._instance:
            return np.full(self.num_vehicles, _INT_MAX)

        vehicles_max_distance = self._instance["vehicles_max_distance"]

        if isinstance(vehicles_max_distance, Number):
            # Some instances describe a uniform max distance as a single
            # value that applies to all vehicles.
            vehicles_max_distance = np.full(
                self.num_vehicles, vehicles_max_distance
            )

        return self._round_func(vehicles_max_distance)

    @cached_property
    def vehicles_max_duration(self) -> np.ndarray:
        if "vehicles_max_duration" not in self._instance:
            return np.full(self.num_vehicles, _INT_MAX)

        vehicles_max_duration = self._instance["vehicles_max_duration"]

        if isinstance(vehicles_max_duration, Number):
            # Some instances describe a uniform max duration as a single
            # value that applies to all vehicles.
            vehicles_max_duration = np.full(
                self.num_vehicles, vehicles_max_duration
            )

        return self._round_func(vehicles_max_duration)

    @cached_property
    def mutually_exclusive_groups(self) -> list[list[int]]:
        if "mutually_exclusive_group" not in self._instance:
            return []

        group_data = self._instance["mutually_exclusive_group"]

        # This assumes groups are numeric, and are numbered {1, 2, ...}.
        raw_groups: list[list[int]] = [[] for _ in range(max(group_data))]
        for client, group in enumerate(group_data):
            raw_groups[group - 1].append(client)

        # Only keep groups if they have more than one member. Empty groups or
        # groups with one member are trivial to decide, so there is no point
        # in keeping them.
        return [group for group in raw_groups if len(group) > 1]


def _depots(instance: Instance) -> list[Depot]:
    """
    Extracts the depots from the VRPLIB instance.
    """
    num_depots = instance.num_depots
    depot_idcs = instance.depot_idcs

    contiguous_lower_idcs = np.arange(num_depots)
    if num_depots == 0 or (depot_idcs != contiguous_lower_idcs).any():
        msg = """
        Source file should contain at least one depot in the contiguous lower
        indices, starting from 1.
        """
        raise ValueError(msg)

    coords = instance.coords

    return [
        Depot(x=coords[idx][0], y=coords[idx][1]) for idx in range(num_depots)
    ]


def _clients(instance: Instance) -> list[Client]:
    """
    Extracts the clients from the VRPLIB instance.
    """
    idx2group: list[Optional[int]] = [None for _ in range(instance.dimension)]
    for group, members in enumerate(instance.mutually_exclusive_groups):
        for client in members:
            idx2group[client] = group

    return [
        Client(
            x=instance.coords[idx][0],
            y=instance.coords[idx][1],
            delivery=instance.demands[idx],
            pickup=instance.backhauls[idx],
            service_duration=instance.service_times[idx],
            tw_early=instance.time_windows[idx][0],
            tw_late=instance.time_windows[idx][1],
            release_time=instance.release_times[idx],
            prize=instance.prizes[idx],
            required=np.isclose(instance.prizes[idx], 0)
            and idx2group[idx] is None,
            group=idx2group[idx],
        )
        for idx in range(instance.num_depots, instance.dimension)
    ]


def _vehicle_types(instance: Instance) -> list[VehicleType]:
    """
    Extracts the vehicle types from the VRPLIB instance.
    """
    vehicles_data = (
        instance.capacities,
        instance.vehicles_allowed_clients,
        instance.vehicles_depots,
        instance.vehicles_max_distance,
        instance.vehicles_max_duration,
    )

    if any(len(arr) != instance.num_vehicles for arr in vehicles_data):
        msg = """
        The number of elements in the vehicles data attributes should be equal
        to the number of vehicles in the problem.
        """
        raise ValueError(msg)

    # VRPLIB instances present data for each available vehicle. We group
    # vehicles by their attributes to create unique vehicle types.
    veh_type2idcs = defaultdict(list)
    for idx, veh_type in enumerate(zip(*vehicles_data)):
        veh_type2idcs[veh_type].append(idx)

    vehicle_types = []
    for type_idx, (attributes, vehicles) in enumerate(veh_type2idcs.items()):
        (capacity, _, depot_idx, max_distance, max_duration) = attributes

        vehicle_type = VehicleType(
            num_available=len(vehicles),
            capacity=capacity,
            start_depot=depot_idx,
            end_depot=depot_idx,
            # The literature specifies depot time windows. We do not have depot
            # time windows but instead set those on the vehicles, generalising
            # the depot time windows.
            tw_early=instance.time_windows[depot_idx][0],
            tw_late=instance.time_windows[depot_idx][1],
            max_duration=max_duration,
            max_distance=max_distance,
            profile=type_idx,
            # A bit hacky, but this csv-like name is really useful to track the
            # actual vehicles that make up this vehicle type.
            name=",".join(map(str, vehicles)),
        )
        vehicle_types.append(vehicle_type)

    return vehicle_types


def _matrices(
    instance: Instance, vehicle_types: list[VehicleType]
) -> tuple[list[np.ndarray], list[np.ndarray]]:
    """
    Extracts the distance and duration matrices from VRPLIB data.
    """
    distances = instance.edge_weight

    if instance.type == "VRPB":
        # In VRPB, linehauls must be served before backhauls. This can be
        # enforced by setting a high value for the distance/duration from depot
        # to backhaul (forcing linehaul to be served first) and a large value
        # from backhaul to linehaul (avoiding linehaul after backhaul clients).
        linehaul = instance.demands > 0
        backhaul = instance.backhauls > 0
        distances[0, backhaul] = MAX_VALUE
        distances[np.ix_(backhaul, linehaul)] = MAX_VALUE

    # Create one distance matrix for each vehicle type.
    distance_matrices = [distances.copy() for _ in range(len(vehicle_types))]

    for type_idx, vehicle_type in enumerate(vehicle_types):
        # A bit hacky, but the vehicle type name tracks the actual vehicles
        # that make up this vehicle type. We parse this to get the allowed
        # clients for this vehicle type.
        vehicle_idx = vehicle_type.name.split(",")[0]
        allowed_clients = instance.vehicles_allowed_clients[int(vehicle_idx)]
        distance_matrix = distance_matrices[type_idx]

        for idx in range(instance.num_depots, instance.dimension):
            if idx not in allowed_clients:
                # Set MAX_VALUE to and from disallowed clients, preventing
                # this vehicle type from serving them.
                distance_matrix[:, idx] = distance_matrix[idx, :] = MAX_VALUE

        np.fill_diagonal(distance_matrix, 0)

    if any(dist.max() > MAX_VALUE for dist in distance_matrices):
        msg = """
        The maximum distance or duration value is very large. This might
        impact numerical stability. Consider rescaling your input data.
        """
        warn(msg, ScalingWarning)

    # VRPLIB instances typically do not have a duration data field, so we
    # assume duration == distance.
    duration_matrices = [matrix.copy() for matrix in distance_matrices]

    return distance_matrices, duration_matrices


def _groups(instance: Instance) -> list[ClientGroup]:
    """
    Extracts the mutually exclusive groups from the VRPLIB instance.
    """
    return [ClientGroup(group) for group in instance.mutually_exclusive_groups]
