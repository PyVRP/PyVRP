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
    ) -> None:
        """
        Creates a time window segment.

        Parameters
        ----------
        idx_first
            Index of the first customer in the route segment.
        idx_last
            Index of the last customer in the route segment.
        duration
            Total duration, including waiting time.
        time_warp
            Total time warp on the route segment.
        tw_early
            Earliest visit moment of the first client.
        tw_late
            Latest visit moment of the last client.
        """
    @overload
    @staticmethod
    def merge(
        dur: Matrix,
        arg0: TimeWindowSegment,
        arg1: TimeWindowSegment,
    ) -> TimeWindowSegment:
        """
        Merges two time window segments, in order.
        """
    @overload
    @staticmethod
    def merge(
        dur: Matrix,
        arg0: TimeWindowSegment,
        arg1: TimeWindowSegment,
        arg2: TimeWindowSegment,
    ) -> TimeWindowSegment:
        """
        Merges three time window segments, in order.
        """
    @overload
    @staticmethod
    def merge(
        dur: Matrix,
        arg0: TimeWindowSegment,
        arg1: TimeWindowSegment,
        arg2: TimeWindowSegment,
        arg3: TimeWindowSegment,
    ) -> TimeWindowSegment:
        """
        Merges four time window segments, in order.
        """
    def total_time_warp(self) -> int:
        """
        Returns the total time warp on this route segment.

        Returns
        -------
        int
            Total time warp on this route segment.
        """
