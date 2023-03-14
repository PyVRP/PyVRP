from typing import overload

class PenaltyBooster:
    """
    Simple penalty booster object that behaves like a context manager. When
    entered, the penalty booster increases the infeasibility penalties of the
    owning penalty manager until the context manager is exited again.
    """

    def __enter__(self) -> PenaltyBooster: ...
    def __exit__(
        self, type: object, value: object, traceback: object
    ) -> None: ...

class PenaltyManager:
    @overload
    def __init__(self, params: PenaltyParams) -> None: ...
    @overload
    def __init__(self) -> None: ...
    def get_penalty_booster(self) -> PenaltyBooster:
        """
        Returns a penalty booster context manager. When entered, the penalty
        booster increases the penalties of this penalty manager until the
        context manager is exited again. See
        :py:attr:`~pyvrp._PenaltyManager.PenaltyParams.repair_booster` for the
        amount by which the penalty terms are increased.

        Examples
        --------

        >>> pm = PenaltyManager(...)
        >>> with pm.get_penalty_booster():
        >>>     ...
        """
    def load_penalty(self, load: int, vehicle_capacity: int) -> int: ...
    def tw_penalty(self, time_warp: int) -> int: ...
    def register_load_feasible(self, is_load_feasible: bool) -> None: ...
    def register_time_feasible(self, is_time_feasible: bool) -> None: ...

class PenaltyParams:
    """
    The penalty manager parameters. These parameters control how the penalties
    are updated throughout the search. See the property descriptions for an
    explanation of the arguments.
    """

    def __init__(
        self,
        init_capacity_penalty: int = ...,
        init_time_warp_penalty: int = ...,
        repair_booster: int = ...,
        num_registrations_between_penalty_updates: int = ...,
        penalty_increase: float = ...,
        penalty_decrease: float = ...,
        target_feasible: float = ...,
    ) -> None: ...
    @property
    def init_capacity_penalty(self) -> int:
        """
        Initial penalty on excess capacity. This is the amount by which one
        unit of excess load capacity is penalised in the objective, at the
        start of the search.

        Returns
        -------
        int
            Initial capacity penalty.
        """
    @property
    def init_time_warp_penalty(self) -> int:
        """
        Initial penalty on time warp. This is the amount by which one unit of
        time warp (time window violations) is penalised in the objective, at
        the start of the search.

        Returns
        -------
        int
            Initial time warp penalty.
        """
    @property
    def num_registrations_between_penalty_updates(self) -> int:
        """
        Number of feasibility registrations between penalty value updates. The
        penalty manager updates the penalty terms every once in a while based
        on recent feasibility registrations. This parameter controls how often
        such updating occurs.

        Returns
        -------
        int
            Number of feasibility registrations between penalty updates.
        """
    @property
    def penalty_decrease(self) -> float:
        """
        Amount :math:`p_d \\in [0, 1]` by which the current penalties are
        decreased when sufficient feasible solutions (see
        :py:attr:`~target_feasible`) have been found amongst the most recent
        registrations. The penalty values :math:`v` are updated as
        :math:`v \\gets p_d v`.

        Returns
        -------
        float
            Penalty decrease parameter :math:`p_d`.
        """
    @property
    def penalty_increase(self) -> float:
        """
        Amount :math:`p_i \\ge 1` by which the current penalties are
        increased when insufficient feasible solutions (see
        :py:attr:`~target_feasible`) have been found amongst the most recent
        registrations. The penalty values :math:`v` are updated as
        :math:`v \\gets p_i v`.

        Returns
        -------
        float
            Penalty increase parameter :math:`p_i`.
        """
    @property
    def repair_booster(self) -> int:
        """
        A repair booster value :math:`r \\ge 1`. This value is used to
        temporarily multiply the current penalty terms, to force feasibility.
        See also
        :meth:`~pyvrp._PenaltyManager.PenaltyManager.get_penalty_booster`.

        Returns
        -------
        int
            The repair booster value :math:`r`.
        """
    @property
    def target_feasible(self) -> float:
        """
        Target percentage :math:`p_f \\in [0, 1]` of feasible registrations
        in the last :py:attr:`~num_registrations_between_penalty_updates`
        registrations. This percentage is used to update the penalty terms:
        when insufficient feasible solutions have been registered, the
        penalties are increased; similarly, when too many feasible solutions
        have been registered, the penalty terms are decreased. This ensures a
        balanced population, with a fraction :math:`p_f` feasible and a fraction
        :math:`1 - p_f` infeasible solutions.

        Returns
        -------
        float
            Target percentage of feasible solutions in the population
            :math:`p_f`.
        """
