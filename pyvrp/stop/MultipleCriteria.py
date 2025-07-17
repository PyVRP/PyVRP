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
        fractions = [crit.fraction_remaining() for crit in self.criteria]
        return min([f for f in fractions if f is not None], default=None)

    def __call__(self, best_cost: float) -> bool:
        return any(crit(best_cost) for crit in self.criteria)
