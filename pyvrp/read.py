import functools
import pathlib
from typing import Callable, Optional, Union

import numpy as np
import vrplib

from .ProblemData import ProblemData

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
    round_func: Optional[Union[str, Callable]] = None,
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
    _round_func = no_rounding
    if isinstance(round_func, str):
        _round_func = ROUND_FUNCS[round_func]
    elif isinstance(round_func, Callable):
        _round_func = round_func
    else:
        assert round_func is None

    instance = vrplib.read_instance(where, instance_format=instance_format)

    # A priori checks
    if len(instance["depot"]) != 1 or instance["depot"][0] != 0:
        raise ValueError(
            "Source file should contain single depot with id 1 "
            + "(depot index should be 0 after subtracting offset 1)"
        )

    num_clients = instance["dimension"]
    num_vehicles = instance.get("vehicles", num_clients - 1)
    capacity = instance.get("capacity", INT_MAX)
    edge_weight = _round_func(instance["edge_weight"])

    if "demand" in instance:
        demands = instance["demand"]
    else:
        demands = np.zeros(num_clients, dtype=int)

    if "node_coord" in instance:
        coords = _round_func(instance["node_coord"])
    else:
        coords = np.zeros((num_clients, 2), dtype=int)

    if "time_window" in instance:
        time_windows = _round_func(instance["time_window"])
    else:
        time_windows = np.repeat([[0, INT_MAX]], num_clients, axis=0)

    if "service_time" in instance:
        service_times = _round_func(instance["service_time"])
    else:
        service_times = np.zeros(num_clients, dtype=int)

    if "release_times" in instance:
        release_times = _round_func(instance["release_times"])
    else:
        release_times = np.zeros(num_clients, dtype=int)

    # Checks after reading/processing data
    if demands[0] != 0:
        raise ValueError("Demand of depot must be 0")
    if time_windows[0, 0] != 0:
        raise ValueError("Depot start of time window must be 0")
    if release_times[0] != 0:
        raise ValueError("Depot release time must be 0")
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
        edge_weight,
        release_times,
    )


ROUND_FUNCS = {
    "trunc": convert_to_int,
    "trunc1": functools.partial(scale_and_truncate_to_decimals, decimals=1),
    "dimacs": functools.partial(scale_and_truncate_to_decimals, decimals=1),
    "none": no_rounding,
}
