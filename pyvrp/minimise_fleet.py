from collections import Counter

from pyvrp._pyvrp import ProblemData, VehicleType
from pyvrp.solve import SolveParams, solve
from pyvrp.stop import FirstFeasible, MultipleCriteria, StoppingCriterion


def minimise_fleet(
    data: ProblemData,
    stop: StoppingCriterion,
    seed: int = 0,
    params: SolveParams = SolveParams(),
) -> list[VehicleType]:
    """
    Attempts to reduce the number of vehicles needed to achieve a feasible
    solution to the given problem instance, subject to a stopping criterion.

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
    list[VehicleType]
        The smallest fleet composition admitting a feasible solution to the
        problem instances that could be found before the stopping criterion was
        hit. The original fleet is returned if no feasible solution was found.
    """
    feas_fleet = data.vehicle_types()

    while True:
        # TODO which vehicle to take out?
        fleet = feas_fleet
        fleet = _vehicles({0: fleet[0].num_available - 1}, fleet)
        data = data.replace(vehicle_types=fleet)

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

        # TODO we can also quit if we have hit a lower bound. See e.g.
        # https://hal.science/hal-00992081/document for ideas

        feas_fleet = fleet
        if res.best.num_routes() < data.num_vehicles:
            # Then we can make a bigger jump because more than one vehicle of
            # the feasible fleet was unused.
            used_types = Counter(r.vehicle_type() for r in res.best.routes())
            feas_fleet = _vehicles(used_types, data.vehicle_types())


def _vehicles(type_counts, vehicle_types):
    return [
        VehicleType(
            type_counts[vehicle_type],
            veh_type.capacity,
            veh_type.start_depot,
            veh_type.end_depot,
            veh_type.fixed_cost,
            veh_type.tw_early,
            veh_type.tw_late,
            veh_type.max_duration,
            veh_type.max_distance,
            veh_type.unit_distance_cost,
            veh_type.unit_duration_cost,
            veh_type.profile,
            name=veh_type.name,
        )
        for vehicle_type, veh_type in zip(type_counts, vehicle_types)
        if type_counts[vehicle_type] > 0
    ]
