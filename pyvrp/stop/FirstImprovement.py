class FirstImprovement:
    """
    Stopping criterion that stops when the first improving solution has been
    found.
    """

    def __call__(self, best_cost: float) -> bool:
        return True
