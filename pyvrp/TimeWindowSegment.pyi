from typing import Union, overload

from .Matrix import DoubleMatrix, IntMatrix

class TimeWindowSegment:
    def __init__(
        self,
        dist: Union[DoubleMatrix, IntMatrix],
        idx_first: int,
        idx_last: int,
        duration: int,
        time_warp: int,
        tw_early: int,
        tw_late: int,
        release: int,
    ) -> None: ...
    @overload
    def merge(self, arg0: TimeWindowSegment) -> TimeWindowSegment: ...
    @overload
    def merge(
        self, arg0: TimeWindowSegment, arg1: TimeWindowSegment
    ) -> TimeWindowSegment: ...
    @overload
    def merge(
        self,
        arg0: TimeWindowSegment,
        arg1: TimeWindowSegment,
        arg2: TimeWindowSegment,
    ) -> TimeWindowSegment: ...
    def segment_time_warp(self) -> int: ...
    def total_time_warp(self) -> int: ...
