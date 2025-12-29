from typing import Generic, TypeVar

T = TypeVar("T")


class RingBuffer(Generic[T]):
    """
    Simple ring buffer structure. Initially the buffer is empty and padded with
    ``None`` values.

    Parameters
    ----------
    maxlen
        Maximum buffer length.
    """

    def __init__(self, maxlen: int):
        self._buffer: list[T | None] = [None for _ in range(maxlen)]
        self._idx = 0

    @property
    def maxlen(self) -> int:
        return len(self._buffer)

    def __len__(self) -> int:
        """
        Returns the number of elements in the ring buffer.
        """
        return sum(val is not None for val in self._buffer)

    def clear(self):
        """
        Clears the ring buffer.
        """
        self._buffer = [None for _ in range(self.maxlen)]
        self._idx = 0

    def append(self, value: T):
        """
        Append to the ring buffer, overwriting the oldest element in the
        buffer.
        """
        self._buffer[self._idx % len(self._buffer)] = value
        self._idx += 1

    def peek(self) -> T | None:
        """
        Returns the next element that will be overwritten when appending
        to the buffer.
        """
        return self._buffer[self._idx % len(self._buffer)]

    def skip(self):
        """
        Skips the next element.
        """
        self._idx += 1
