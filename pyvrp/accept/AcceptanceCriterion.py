from typing import Protocol


class AcceptanceCriterion(Protocol):
    def __call__(self, best: float, current: float, candidate: float) -> bool:
        ...
