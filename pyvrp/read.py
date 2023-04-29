import functools
import pathlib
from typing import Callable, Dict, List, Union

import numpy as np
import vrplib

from ._ProblemData import ProblemData

_Routes = List[List[int]]
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
ROUND_FUNCS: Dict[str, _RoundingFunc] = {
    "round": round_nearest,
    "trunc": convert_to_int,
    "trunc1": functools.partial(scale_and_truncate_to_decimals, decimals=1),
    "dimacs": functools.partial(scale_and_truncate_to_decimals, decimals=1),
    "none": no_rounding,
}


def read(
    where: Union[str, pathlib.Path],
    instance_format: str = "vrplib",
    round_func: Union[str, _RoundingFunc] = no_rounding,
) -> ProblemData:
    """
    Reads the VRPLIB file at the given location, and returns a ProblemData
    instance.

    Parameters
    ----------
    where
        File location to read. Assumes the data on the given location is in
        VRPLIB format.
    instance_format, optional
        File format of the instance to read, one of ``'vrplib'`` (default) or
        ``'solomon'``.
    round_func, optional
        Optional rounding function. Will be applied to round data if the data
        is not already integer. This can either be a function or a string:

            * ``'round'`` rounds the values to the nearest integer;
            * ``'trunc'`` truncates the values to be integral;
            * ``'trunc1'`` or ``'dimacs'`` scale and truncate to the nearest
              decimal;
            * ``'none'`` does no rounding. This is the default.

    Returns
    -------
    ProblemData
        Data instance constructed from the read data.
    """
    if isinstance(round_func, str) and round_func in ROUND_FUNCS:
        round_func = ROUND_FUNCS[round_func]
    elif not callable(round_func):
        raise ValueError(
            f"round_func = {round_func} is not understood. Can be a function,"
            f" or one of {ROUND_FUNCS.keys()}."
        )

    instance = vrplib.read_instance(where, instance_format=instance_format)

    # A priori checks
    if "dimension" in instance:
        num_clients = instance["dimension"]
    else:
        if "demand" not in instance:
            raise ValueError("File should either contain dimension or demands")
        num_clients = len(instance["demand"])

    depots = instance.get("depot", np.array([0]))
    num_vehicles = instance.get("vehicles", num_clients - 1)
    capacity = instance.get("capacity", _INT_MAX)
    edge_weight = round_func(instance["edge_weight"])

    if "demand" in instance:
        demands = instance["demand"]
    else:
        demands = np.zeros(num_clients, dtype=int)

    if "node_coord" in instance:
        coords = round_func(instance["node_coord"])
    else:
        coords = np.zeros((num_clients, 2), dtype=int)

    if "service_time" in instance:
        service_times = round_func(instance["service_time"])
    else:
        service_times = np.zeros(num_clients, dtype=int)

    if "time_window" in instance:
        time_windows = round_func(instance["time_window"])
    else:
        # The default value for the latest time window based on the maximum
        # route duration. This ensures that the time window constraints are
        # always satisfied.
        bound = num_clients * (edge_weight.max() + service_times.max())
        bound = min(bound, _INT_MAX)
        time_windows = np.repeat([[0, bound]], num_clients, axis=0)

    # Checks
    if len(depots) != 1 or depots[0] != 0:
        raise ValueError(
            "Source file should contain single depot with index 1 "
            + "(depot index should be 0 after subtracting offset 1)"
        )

    if demands[0] != 0:
        raise ValueError("Demand of depot must be 0")

    if time_windows[0, 0] != 0:
        raise ValueError("Depot start of time window must be 0")

    if service_times[0] != 0:
        raise ValueError("Depot service duration must be 0")

    if (time_windows[:, 0] > time_windows[:, 1]).any():
        raise ValueError("Time window cannot start after end")

    return ProblemData(
        coords,
        demands,
        num_vehicles,
        capacity,
        time_windows,
        service_times,
        edge_weight,  # distance and duration are assumed to be the same in
        edge_weight,  # the VRPLIB instances we read here.
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
