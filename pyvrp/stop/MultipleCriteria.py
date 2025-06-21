from .StoppingCriterion import StoppingCriterion


class MultipleCriteria:
    """
    Simple aggregate class that manages multiple stopping criteria at once.
    """

    def __init__(self, criteria: list[StoppingCriterion]) -> None:
        if len(criteria) == 0:
            raise ValueError("Expected one or more stopping criteria.")

        self.criteria = criteria

    def fraction_remaining(self) -> float | None:
        fractions = [
            crit.fraction_remaining()
            for crit in self.criteria
            if crit.fraction_remaining() is not None
        ]
        return min(fractions) if fractions else None  # type: ignore

    def __call__(self, best_cost: float) -> bool:
        return any(crit(best_cost) for crit in self.criteria)
