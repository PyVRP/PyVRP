from dataclasses import dataclass
from statistics import fmean

import numpy as np

from pyvrp._pyvrp import CostEvaluator, Solution


@dataclass
class PenaltyParams:
    """
    The penalty manager parameters.

    Parameters
    ----------
    init_load_penalty
        Initial penalty on excess load. This is the amount by which one unit of
        excess load is penalised in the objective, at the start of the search.
    init_time_warp_penalty
        Initial penalty on time warp. This is the amount by which one unit of
        time warp (time window violations) is penalised in the objective, at
        the start of the search.
    init_dist_penalty
        Initial penalty on excess distance. This is the amount by which one
        unit of excess distance is penalised in the objective, at the start of
        the search.
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
    init_load_penalty
        Initial penalty on excess load.
    init_time_warp_penalty
        Initial penalty on time warp.
    init_dist_penalty
        Initial penalty on excess distance.
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

    init_load_penalty: int = 20
    init_time_warp_penalty: int = 6
    init_dist_penalty: int = 6
    repair_booster: int = 12
    solutions_between_updates: int = 50
    penalty_increase: float = 1.34
    penalty_decrease: float = 0.32
    target_feasible: float = 0.43

    def __post_init__(self):
        if not self.penalty_increase >= 1.0:
            raise ValueError("Expected penalty_increase >= 1.")

        if not 0.0 <= self.penalty_decrease <= 1.0:
            raise ValueError("Expected penalty_decrease in [0, 1].")

        if not 0.0 <= self.target_feasible <= 1.0:
            raise ValueError("Expected target_feasible in [0, 1].")

        if not self.repair_booster >= 1.0:
            raise ValueError("Expected repair_booster >= 1.")


class PenaltyManager:
    """
    Creates a PenaltyManager instance.

    This class manages time warp and load penalties, and provides penalty terms
    for given time warp and load values. It updates these penalties based on
    recent history, and can be used to provide a temporary penalty booster
    object that increases the penalties for a short duration.

    Parameters
    ----------
    params
        PenaltyManager parameters. If not provided, a default will be used.
    """

    def __init__(self, params: PenaltyParams = PenaltyParams()):
        self._params = params

        self._penalties = np.array(
            [
                params.init_load_penalty,
                params.init_time_warp_penalty,
                params.init_dist_penalty,
            ]
        )

        self._feas_lists: list[list[bool]] = [
            [],  # tracks recent load feasibility
            [],  # track recent time feasibility
            [],  # track recent distance feasibility
        ]

    def _compute(
        self,
        penalty: int,
        feas_percentage: float,
        tolerance: float = 0.05,
    ) -> int:
        # Computes and returns the new penalty value, given the current value
        # and the percentage of feasible solutions since the last update.
        diff = self._params.target_feasible - feas_percentage

        if abs(diff) < tolerance:
            return penalty

        # +/- 1 to ensure we do not get stuck at the same integer values.
        if diff > 0:
            new_penalty = self._params.penalty_increase * penalty + 1
        else:
            new_penalty = self._params.penalty_decrease * penalty - 1

        return int(np.clip(new_penalty, 1, 100_000))

    def _register(self, feas_list: list[bool], penalty: int, is_feas: bool):
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
        args = [
            not sol.has_excess_load(),
            not sol.has_time_warp(),
            not sol.has_excess_distance(),
        ]

        for idx, is_feas in enumerate(args):
            feas_list = self._feas_lists[idx]
            penalty = self._penalties[idx]
            self._penalties[idx] = self._register(feas_list, penalty, is_feas)

    def cost_evaluator(self) -> CostEvaluator:
        """
        Get a cost evaluator using the current penalty values.
        """
        return CostEvaluator(*self._penalties)

    def booster_cost_evaluator(self) -> CostEvaluator:
        """
        Get a cost evaluator using the boosted current penalty values.
        """
        return CostEvaluator(*(self._penalties * self._params.repair_booster))
