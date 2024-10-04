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
        problem instance that could be found before the stopping criterion was
        hit. The original fleet is returned if no feasible solution was found.

    Raises
    ------
    ValueError
        When the instance contains more than one vehicle type. That setting is
        not yet supported. Alternatively, when the instance contains optional
        clients. This method attempts to find a good upper bound on the number
        of vehicles needed to solve the complete problem.
    """
    if data.num_vehicle_types != 1:
        msg = "Fleet minimisation does not understand multiple vehicle types."
        raise ValueError(msg)

    if any(not client.required for client in data.clients()):
        msg = "Fleet minimisation does not work with optional clients."
        raise ValueError(msg)

    feas_fleet = data.vehicle_type(0)
    lower_bound = _lower_bound(data)

    while feas_fleet.num_available > lower_bound:
        # Reduce feasible fleet by one vehicle, and retry solving.
        fleet = feas_fleet.replace(num_available=feas_fleet.num_available - 1)
        data = data.replace(vehicle_types=[fleet])

        res = solve(
            data,
            stop=MultipleCriteria([FirstFeasible(), stop]),
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
            feas_fleet = fleet.replace(num_available=res.best.num_routes())

    return feas_fleet


def _lower_bound(data: ProblemData) -> int:
    vehicle_type = data.vehicle_type(0)

    # TODO additional simple bounding techniques exist, for example based on
    # time properties. See https://doi.org/10.1007/978-3-319-07046-9_30 for
    # details.

    # Computes a bound based on packing delivery or pickup demands in the given
    # vehicles over all load dimensions. The strongest bound is returned.
    bound = 0
    for dim in range(data.num_load_dimensions):
        delivery = sum(c.delivery[dim] for c in data.clients())
        pickup = sum(c.pickup[dim] for c in data.clients())
        demand = max(delivery, pickup)
        capacity = vehicle_type.capacity[dim]

        bound = max(int(np.ceil(demand / max(capacity, 1))), bound)

    return bound
