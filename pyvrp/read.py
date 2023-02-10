import functools
import pathlib
from typing import Callable, Dict, Union

import numpy as np
import vrplib

from .ProblemData import ProblemData

_RoundingFunc = Callable[[np.ndarray], np.ndarray]

INT_MAX = np.iinfo(np.int32).max


def convert_to_int(vals: np.ndarray):
    return vals.astype(int)


def scale_and_truncate_to_decimals(vals: np.ndarray, decimals: int = 0):
    return (vals * (10**decimals)).astype(int)


def no_rounding(vals):
    return vals


def read(
    where: Union[str, pathlib.Path],
    instance_format: str = "vrplib",
    round_func: Union[str, _RoundingFunc] = no_rounding,
) -> ProblemData:
    """
    Reads the (C)VRPLIB file at the given location, and returns a ProblemData
    instance.

    Parameters
    ----------
    where
        File location to read. Assumes the data on the given location is in
        (C)VRPLIB format.
    instance_format, optional
        File format of the instance to read. Currently supported are 'vrplib'
        (default) and 'solomon'.
    round_func, optional
        Function to apply to distances/durations, node coordinates, time
        windows and service times to specify how to round to integer. Required
        if data is not already integer.

    Returns
    -------
    ProblemData
        Data instance constructed from the read data.
    """
    if isinstance(round_func, str):
        round_func = ROUND_FUNCS[round_func]

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
    capacity = instance.get("capacity", INT_MAX)
    edge_weight = round_func(instance["edge_weight"])

    if "demand" in instance:
        demands = instance["demand"]
    else:
        demands = np.zeros(num_clients, dtype=int)

    if "node_coord" in instance:
        coords = round_func(instance["node_coord"])
    else:
        coords = np.zeros((num_clients, 2), dtype=int)

    if "time_window" in instance:
        time_windows = round_func(instance["time_window"])
    else:
        time_windows = np.repeat([[0, INT_MAX]], num_clients, axis=0)

    if "service_time" in instance:
        service_times = round_func(instance["service_time"])
    else:
        service_times = np.zeros(num_clients, dtype=int)

    if "release_time" in instance:
        release_times = round_func(instance["release_time"])
    else:
        release_times = np.zeros(num_clients, dtype=int)

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
    if release_times[0] != 0:
        raise ValueError("Depot release time must be 0")
    if service_times[0] != 0:
        raise ValueError("Depot service duration must be 0")
    if release_times[0] != 0:
        raise ValueError("Depot release time must be 0")
    if (time_windows[:, 0] > time_windows[:, 1]).any():
        raise ValueError("Time window cannot start after end")

    return ProblemData(
        coords,
        demands,
        num_vehicles,
        capacity,
        time_windows,
        service_times,
        edge_weight,
        release_times,
    )


ROUND_FUNCS: Dict[str, _RoundingFunc] = {
    "trunc": convert_to_int,
    "trunc1": functools.partial(scale_and_truncate_to_decimals, decimals=1),
    "dimacs": functools.partial(scale_and_truncate_to_decimals, decimals=1),
    "none": no_rounding,
}
