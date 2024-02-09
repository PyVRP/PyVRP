import functools
import pathlib
from numbers import Number
from typing import Callable, Union
from warnings import warn

import numpy as np
import vrplib

from pyvrp._pyvrp import Client, ProblemData, VehicleType
from pyvrp.constants import MAX_USER_VALUE
from pyvrp.exceptions import ScalingWarning

_Routes = list[list[int]]
_RoundingFunc = Callable[[np.ndarray], np.ndarray]

_INT_MAX = np.iinfo(np.int32).max


def round_nearest(vals: np.ndarray):
    return np.round(vals).astype(int)


def convert_to_int(vals: np.ndarray):
    return vals.astype(int)


def scale_and_truncate_to_decimals(vals: np.ndarray, decimals: int = 0):
    return (vals * (10**decimals)).astype(int)


def no_rounding(vals):
    return vals


INSTANCE_FORMATS = ["vrplib", "solomon"]
ROUND_FUNCS: dict[str, _RoundingFunc] = {
    "round": round_nearest,
    "trunc": convert_to_int,
    "trunc1": functools.partial(scale_and_truncate_to_decimals, decimals=1),
    "dimacs": functools.partial(scale_and_truncate_to_decimals, decimals=1),
    "none": no_rounding,
}


def read(
    where: Union[str, pathlib.Path],
    instance_format: str = "vrplib",
    round_func: Union[str, _RoundingFunc] = "none",
) -> ProblemData:
    """
    Reads the VRPLIB file at the given location, and returns a ProblemData
    instance.

    .. note::

       See the
       :doc:`VRPLIB format explanation <../dev/supported_vrplib_fields>` page
       for more details.

    Parameters
    ----------
    where
        File location to read. Assumes the data on the given location is in
        VRPLIB format.
    instance_format
        File format of the instance to read, one of ``'vrplib'`` (default) or
        ``'solomon'``.
    round_func
        Optional rounding function. Will be applied to round data if the data
        is not already integer. This can either be a function or a string:

            * ``'round'`` rounds the values to the nearest integer;
            * ``'trunc'`` truncates the values to be integral;
            * ``'trunc1'`` or ``'dimacs'`` scale and truncate to the nearest
              decimal;
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

    instance = vrplib.read_instance(where, instance_format=instance_format)

    # A priori checks
    if "dimension" in instance:
        dimension: int = instance["dimension"]
    else:
        if "demand" not in instance:
            raise ValueError("File should either contain dimension or demands")
        dimension = len(instance["demand"])

    depots: np.ndarray = instance.get("depot", np.array([0]))
    num_vehicles: int = instance.get("vehicles", dimension - 1)
    capacity: int = instance.get("capacity", _INT_MAX)

    # If this value is supplied, we should pass it through the round func and
    # then unwrap the result. If it's not given, the default value is None,
    # which PyVRP understands.
    max_duration: int = instance.get("vehicles_max_duration", _INT_MAX)
    if max_duration != _INT_MAX:
        max_duration = round_func(np.array([max_duration])).item()

    distances: np.ndarray = round_func(instance["edge_weight"])

    if "demand" in instance:
        demands: np.ndarray = instance["demand"]
    else:
        demands = np.zeros(dimension, dtype=int)

    if "node_coord" in instance:
        coords: np.ndarray = round_func(instance["node_coord"])
    else:
        coords = np.zeros((dimension, 2), dtype=int)

    if "service_time" in instance:
        if isinstance(instance["service_time"], Number):
            # Some instances describe a uniform service time as a single value
            # that applies to all clients.
            service_times = np.full(dimension, instance["service_time"])
            service_times[0] = 0
            service_times = round_func(service_times)
        else:
            service_times = round_func(instance["service_time"])
    else:
        service_times = np.zeros(dimension, dtype=int)

    if "time_window" in instance:
        # VRPLIB instances typically do not have a duration data field, so we
        # assume duration == distance if the instance has time windows.
        durations = distances
        time_windows: np.ndarray = round_func(instance["time_window"])
    else:
        # No time window data. So the time window component is not relevant,
        # and we set all time-related fields to zero.
        durations = np.zeros_like(distances)
        service_times = np.zeros(dimension, dtype=int)
        time_windows = np.zeros((dimension, 2), dtype=int)
        time_windows[:, 1] = np.iinfo(np.int32).max

    if "vehicles_depot" in instance:
        items: list[list[int]] = [[] for _ in depots]
        for vehicle, depot in enumerate(instance["vehicles_depot"], 1):
            items[depot - 1].append(vehicle)

        depot_vehicle_pairs = items
    else:
        depot_vehicle_pairs = [[idx + 1 for idx in range(num_vehicles)]]

    if "release_time" in instance:
        release_times: np.ndarray = round_func(instance["release_time"])
    else:
        release_times = np.zeros(dimension, dtype=int)

    prizes = round_func(instance.get("prize", np.zeros(dimension, dtype=int)))

    # Checks
    if len(depots) == 0 or (depots != np.arange(len(depots))).any():
        msg = """
        Source file should contain at least one depot in the contiguous lower
        indices, starting from 1.
        """
        raise ValueError(msg)

    if max(distances.max(), durations.max()) > MAX_USER_VALUE:
        msg = """
        The maximum distance or duration value is very large. This might
        impact numerical stability. Consider rescaling your input data.
        """
        warn(msg, ScalingWarning)

    clients = [
        Client(
            coords[idx][0],  # x
            coords[idx][1],  # y
            demands[idx],
            service_times[idx],
            time_windows[idx][0],  # TW early
            time_windows[idx][1],  # TW late
            release_times[idx],
            prizes[idx],
            np.isclose(prizes[idx], 0),  # required only when prize is zero
        )
        for idx in range(dimension)
    ]

    vehicle_types = [
        VehicleType(
            len(vehicles),
            capacity,
            depot_idx,
            max_duration=max_duration,
            # A bit hacky, but this csv-like name is really useful to track the
            # actual vehicles that make up this vehicle type.
            name=",".join(map(str, vehicles)),
        )
        for depot_idx, vehicles in enumerate(depot_vehicle_pairs)
    ]

    return ProblemData(
        clients[len(depots) :],
        clients[: len(depots)],
        vehicle_types,
        distances,
        durations,
    )


def read_solution(where: Union[str, pathlib.Path]) -> _Routes:
    """
    Reads a solution in VRPLIB format from file at the given location, and
    returns the routes contained in it.

    Parameters
    ----------
    where
        File location to read. Assumes the solution in the file on the given
        location is in VRPLIB solution format.

    Returns
    -------
    list
        List of routes, where each route is a list of client numbers.
    """
    sol = vrplib.read_solution(str(where))
    return sol["routes"]  # type: ignore
