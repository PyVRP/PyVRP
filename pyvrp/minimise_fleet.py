import numpy as np

from pyvrp._pyvrp import ProblemData, VehicleType
from pyvrp.solve import SolveParams, solve
from pyvrp.stop import FirstFeasible, MultipleCriteria, StoppingCriterion


def minimise_fleet(
    data: ProblemData,
    stop: StoppingCriterion,
    seed: int = 0,
    params: SolveParams = SolveParams(),
) -> VehicleType:
    """
    Attempts to reduce the number of vehicles needed to achieve a feasible
    solution to the given problem instance, subject to a stopping criterion.

    .. warning::

       This function is currently unable to solve instances with multiple
       vehicle types. Support for such a setting may be added in future
       versions of PyVRP.

    Parameters
    ----------
    data
        Problem instance with a given vehicle composition.
    stop
        Stopping criterion that determines how much effort to spend on finding
        smaller fleet compositions.
    seed
        Seed value to use for the random number stream. Default 0.
    params
        Solver parameters to use. If not provided, a default will be used.

    Returns
    -------
    VehicleType
        The smallest fleet composition admitting a feasible solution to the
        problem instances that could be found before the stopping criterion was
        hit. The original fleet is returned if no feasible solution was found.

    Raises
    ------
    ValueError
        When the instance contains more than one vehicle type. That setting is
        not yet supported.
    """
    if data.num_vehicle_types != 1:
        msg = "Fleet minimisation does not understand multiple vehicle types."
        raise ValueError(msg)

    feas_fleet = data.vehicle_type(0)
    lower_bound = _lower_bound(data)

    while feas_fleet.num_available > lower_bound:
        # Reduce from feasible fleet by one vehicle, and retry solving that
        # reduced instance.
        fleet = _vehicles(feas_fleet.num_available - 1, feas_fleet)
        data = data.replace(vehicle_types=[fleet])

        res = solve(
            data,
            stop=MultipleCriteria([stop, FirstFeasible()]),
            seed=seed,
            collect_stats=False,
            display=False,
            params=params,
        )

        if stop(res.cost()) or not res.is_feasible():
            return feas_fleet

        feas_fleet = fleet
        if res.best.num_routes() < data.num_vehicles:
            # Then we can make a bigger jump because more than one vehicle of
            # the feasible fleet was unused.
            feas_fleet = _vehicles(res.best.num_routes(), fleet)

    return feas_fleet


def _vehicles(num_available: int, vehicle_type: VehicleType) -> VehicleType:
    return VehicleType(
        num_available,
        vehicle_type.capacity,
        vehicle_type.start_depot,
        vehicle_type.end_depot,
        vehicle_type.fixed_cost,
        vehicle_type.tw_early,
        vehicle_type.tw_late,
        vehicle_type.max_duration,
        vehicle_type.max_distance,
        vehicle_type.unit_distance_cost,
        vehicle_type.unit_duration_cost,
        vehicle_type.profile,
        name=vehicle_type.name,
    )


def _lower_bound(data: ProblemData) -> int:
    vehicle_type = data.vehicle_type(0)

    # TODO additional simple bounding techniques exist, for example based on
    # time properties. See https://doi.org/10.1007/978-3-319-07046-9_30 for
    # details.

    # Computes a simple bound based on packing delivery or pickup demand in the
    # given vehicles.
    delivery = sum(c.delivery for c in data.clients())
    pickup = sum(c.pickup for c in data.clients())
    demand = max(delivery, pickup)
    return int(np.ceil(demand / max(vehicle_type.capacity, 1)))
