import time
from collections import Counter

from pyvrp._pyvrp import ProblemData, VehicleType
from pyvrp.solve import SolveParams, solve
from pyvrp.stop import FirstFeasible, MaxRuntime, MultipleCriteria


def minimise_fleet(
    data: ProblemData,
    max_runtime: MaxRuntime,
    seed: int = 0,
    params: SolveParams = SolveParams(),
) -> list[VehicleType]:
    """
    Attempts to reduce the necessary vehicles to achieve a feasible solution
    to the given problem instance, within the given runtime budget.

    Parameters
    ----------
    data
        Problem instance with a given vehicle composition.
    max_runtime
        Maximum runtime to spend reducing the fleet size.

    Returns
    -------
    list[VehicleType]
        The smallest fleet composition admitting a feasible solution to the
        problem instances that could be found in the given amount of runtime.
        If no feasible solution could be found, the original fleet composition
        is returned.
    """
    start = time.perf_counter()
    end = start + max_runtime.max_runtime
    feas_fleet = fleet = data.vehicle_types()

    while (remaining := end - time.perf_counter()) >= 0:
        stop = MultipleCriteria([MaxRuntime(remaining), FirstFeasible()])

        # Take one vehicle out.
        # TODO which?
        fleet = feas_fleet
        fleet = _vehicles({0: fleet[0].num_available - 1}, fleet)
        data = data.replace(vehicle_types=fleet)

        res = solve(
            data,
            stop=stop,
            seed=seed,
            collect_stats=False,
            display=False,
            params=params,
        )

        if not res.is_feasible():  # then we return the last feasible fleet
            return feas_fleet

        feas_fleet = fleet
        if res.best.num_routes() < data.num_vehicles:
            # Then we can make a bigger jump because more than one vehicle of
            # the feasible fleet was unused.
            used_types = Counter(r.vehicle_type() for r in res.best.routes())
            feas_fleet = _vehicles(used_types, data.vehicle_types())

    return feas_fleet


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
