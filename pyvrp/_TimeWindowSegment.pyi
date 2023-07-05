from typing import overload

from ._Matrix import Matrix

class TimeWindowSegment:
    def __init__(
        self,
        idx_first: int,
        idx_last: int,
        duration: int,
        time_warp: int,
        tw_early: int,
        tw_late: int,
        release_time: int,
    ) -> None:
        """
        Creates a time window segment.

        Parameters
        ----------
        idx_first
            Index of the first client in the route segment.
        idx_last
            Index of the last client in the route segment.
        duration
            Total duration, including waiting time.
        time_warp
            Total time warp on the route segment.
        tw_early
            Earliest visit moment of the first client.
        tw_late
            Latest visit moment of the first client.
        release_time
            Earliest moment to start the route segment.
        """
    @overload
    @staticmethod
    def merge(
        duration_matrix: Matrix,
        first: TimeWindowSegment,
        second: TimeWindowSegment,
    ) -> TimeWindowSegment:
        """
        Merges two time window segments, in order.
        """
    @overload
    @staticmethod
    def merge(
        duration_matrix: Matrix,
        first: TimeWindowSegment,
        second: TimeWindowSegment,
        third: TimeWindowSegment,
    ) -> TimeWindowSegment:
        """
        Merges three time window segments, in order.
        """
    def total_time_warp(self) -> int:
        """
        Returns the total time warp on this route segment.

        Returns
        -------
        int
            Total time warp on this route segment.
        """
