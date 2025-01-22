from __future__ import annotations

from dataclasses import dataclass
from statistics import fmean
from warnings import warn

import numpy as np

from pyvrp._pyvrp import CostEvaluator, ProblemData, Solution
from pyvrp.exceptions import PenaltyBoundWarning


@dataclass
class PenaltyParams:
    """
    The penalty manager parameters.

    Parameters
    ----------
    repair_booster
        A repair booster value :math:`r \\ge 1`. This value is used to
        temporarily multiply the current penalty terms, to force feasibility.
        See also
        :meth:`~pyvrp.PenaltyManager.PenaltyManager.booster_cost_evaluator`.
    solutions_between_updates
        Number of feasibility registrations between penalty value updates. The
        penalty manager updates the penalty terms every once in a while based
        on recent feasibility registrations. This parameter controls how often
        such updating occurs.
    penalty_increase
        Amount :math:`p_i \\ge 1` by which the current penalties are
        increased when insufficient feasible solutions (see
        ``target_feasible``) have been found amongst the most recent
        registrations. The penalty values :math:`v` are updated as
        :math:`v \\gets p_i v`.
    penalty_decrease
        Amount :math:`p_d \\in [0, 1]` by which the current penalties are
        decreased when sufficient feasible solutions (see ``target_feasible``)
        have been found amongst the most recent registrations. The penalty
        values :math:`v` are updated as :math:`v \\gets p_d v`.
    target_feasible
        Target percentage :math:`p_f \\in [0, 1]` of feasible registrations
        in the last ``solutions_between_updates`` registrations. This
        percentage is used to update the penalty terms: when insufficient
        feasible solutions have been registered, the penalties are increased;
        similarly, when too many feasible solutions have been registered, the
        penalty terms are decreased. This ensures a balanced population, with a
        fraction :math:`p_f` feasible and a fraction :math:`1 - p_f` infeasible
        solutions.

    Attributes
    ----------
    repair_booster
        A repair booster value.
    solutions_between_updates
        Number of feasibility registrations between penalty value updates.
    penalty_increase
        Amount :math:`p_i \\ge 1` by which the current penalties are
        increased when insufficient feasible solutions (see
        ``target_feasible``) have been found amongst the most recent
        registrations.
    penalty_decrease
        Amount :math:`p_d \\in [0, 1]` by which the current penalties are
        decreased when sufficient feasible solutions (see ``target_feasible``)
        have been found amongst the most recent registrations.
    target_feasible
        Target percentage :math:`p_f \\in [0, 1]` of feasible registrations
        in the last ``solutions_between_updates`` registrations.
    """

    repair_booster: int = 12
    solutions_between_updates: int = 50
    penalty_increase: float = 1.34
    penalty_decrease: float = 0.32
    target_feasible: float = 0.43

    def __post_init__(self):
        if not self.repair_booster >= 1:
            raise ValueError("Expected repair_booster >= 1.")

        if not self.solutions_between_updates >= 1:
            raise ValueError("Expected solutions_between_updates >= 1.")

        if not self.penalty_increase >= 1.0:
            raise ValueError("Expected penalty_increase >= 1.")

        if not (0.0 <= self.penalty_decrease <= 1.0):
            raise ValueError("Expected penalty_decrease in [0, 1].")

        if not (0.0 <= self.target_feasible <= 1.0):
            raise ValueError("Expected target_feasible in [0, 1].")


class PenaltyManager:
    """
    Creates a PenaltyManager instance.

    This class manages time warp and load penalties, and provides penalty terms
    for given time warp and load values. It updates these penalties based on
    recent history, and can be used to provide a temporary penalty booster
    object that increases the penalties for a short duration.

    .. note::

       Consider initialising using :meth:`~init_from` to compute initial
       penalty values that are scaled according to the data instance.

    Parameters
    ----------
    initial_penalties
        Initial penalty values for units of load (idx 0), duration (1), and
        distance (2) violations. These values are clipped to the range
        ``[MIN_PENALTY, MAX_PENALTY]``.
    params
        PenaltyManager parameters. If not provided, a default will be used.
    """

    MIN_PENALTY: float = 0.1
    MAX_PENALTY: float = 100_000.0
    FEAS_TOL: float = 0.05

    def __init__(
        self,
        initial_penalties: tuple[list[float], float, float],
        params: PenaltyParams = PenaltyParams(),
    ):
        self._params = params
        self._penalties = np.clip(
            initial_penalties[0] + list(initial_penalties[1:]),
            self.MIN_PENALTY,
            self.MAX_PENALTY,
        )

        # Tracks recent feasibilities for each penalty dimension.
        self._feas_lists: list[list[bool]] = [
            [] for _ in range(len(self._penalties))
        ]

    def penalties(self) -> tuple[list[float], float, float]:
        """
        Returns the current penalty values.
        """
        return (
            self._penalties[:-2].tolist(),  # loads
            self._penalties[-2],  # duration
            self._penalties[-1],  # distance
        )

    @classmethod
    def init_from(
        cls,
        data: ProblemData,
        params: PenaltyParams = PenaltyParams(),
    ) -> PenaltyManager:
        """
        Initialises from the given data instance and parameter object. The
        initial penalty values are computed from the problem data.

        Parameters
        ----------
        data
            Data instance to use when computing penalty values.
        params
            PenaltyManager parameters. If not provided, a default will be used.
        """
        distances = data.distance_matrices()
        durations = data.duration_matrices()

        # We first determine the elementwise minimum cost across all vehicle
        # types. This is the cheapest way any edge can be traversed.
        unique_edge_costs = {
            (
                veh_type.unit_distance_cost,
                veh_type.unit_duration_cost,
                veh_type.profile,
            )
            for veh_type in data.vehicle_types()
        }

        first, *rest = unique_edge_costs
        unit_dist, unit_dur, prof = first
        edge_costs = unit_dist * distances[prof] + unit_dur * durations[prof]
        for unit_dist, unit_dur, prof in rest:
            mat = unit_dist * distances[prof] + unit_dur * durations[prof]
            np.minimum(edge_costs, mat, out=edge_costs)

        # Best edge cost/distance/duration over all vehicle types and profiles,
        # and then average that for the entire matrix to obtain an "average
        # best" edge cost/distance/duration.
        avg_cost = edge_costs.mean()
        avg_distance = np.minimum.reduce(distances).mean()
        avg_duration = np.minimum.reduce(durations).mean()

        avg_load = np.zeros((data.num_load_dimensions,))
        if data.num_clients != 0 and data.num_load_dimensions != 0:
            pickups = np.array([c.pickup for c in data.clients()])
            deliveries = np.array([c.delivery for c in data.clients()])
            avg_load = np.maximum(pickups, deliveries).mean(axis=0)

        # Initial penalty parameters are meant to weigh an average increase
        # in the relevant value by the same amount as the average edge cost.
        init_load = avg_cost / np.maximum(avg_load, 1)
        init_tw = avg_cost / max(avg_duration, 1)
        init_dist = avg_cost / max(avg_distance, 1)
        return cls((init_load.tolist(), init_tw, init_dist), params)

    def _compute(self, penalty: float, feas_percentage: float) -> float:
        # Computes and returns the new penalty value, given the current value
        # and the percentage of feasible solutions since the last update.
        diff = self._params.target_feasible - feas_percentage

        if abs(diff) < self.FEAS_TOL:
            return penalty

        if diff > 0:
            new_penalty = self._params.penalty_increase * penalty
        else:
            new_penalty = self._params.penalty_decrease * penalty

        if new_penalty >= self.MAX_PENALTY:
            msg = """
            A penalty parameter has reached its maximum value. This means PyVRP
            struggles to find a feasible solution for the instance that's being
            solved, either because the instance has no feasible solution, or it
            is very hard to find one. Check the instance carefully to determine
            if a feasible solution exists.
            """
            warn(msg, PenaltyBoundWarning)

        return np.clip(new_penalty, self.MIN_PENALTY, self.MAX_PENALTY)

    def _register(self, feas_list: list[bool], penalty: float, is_feas: bool):
        feas_list.append(is_feas)

        if len(feas_list) != self._params.solutions_between_updates:
            return penalty

        avg = fmean(feas_list)
        feas_list.clear()
        return self._compute(penalty, avg)

    def register(self, sol: Solution):
        """
        Registers the feasibility dimensions of the given solution.
        """
        is_feasible = [
            *[excess == 0 for excess in sol.excess_load()],
            not sol.has_time_warp(),
            not sol.has_excess_distance(),
        ]

        for idx, is_feas in enumerate(is_feasible):
            feas_list = self._feas_lists[idx]
            penalty = self._penalties[idx]
            self._penalties[idx] = self._register(feas_list, penalty, is_feas)

    def cost_evaluator(self) -> CostEvaluator:
        """
        Get a cost evaluator using the current penalty values.
        """
        *loads, tw, dist = self._penalties
        return CostEvaluator(loads, tw, dist)

    def booster_cost_evaluator(self) -> CostEvaluator:
        """
        Get a cost evaluator using the boosted current penalty values.
        """
        *loads, tw, dist = self._penalties * self._params.repair_booster
        return CostEvaluator(loads, tw, dist)
