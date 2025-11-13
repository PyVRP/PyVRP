class ReachedBKS:
    """Indicates that the best known solution has been reached.

    This stopping criterion can be used to terminate the optimization
    process once a known best solution has been found. This is useful
    in scenarios where the optimal solution is known beforehand, allowing
    the algorithm to stop early and save computational resources.

    Parameters
    ----------
    bks_value
        The objective value of the best known solution.
    """

    def __init__(self, bks_value: int):
        """Initializes the ReachedBKS stopping criterion.

        Parameters
        ----------
        bks_value
            The objective value of the best known solution. Must be a 
            positive integer (PyVRP uses integer costs internally).
        """
        if bks_value <= 0:
            raise ValueError("bks_value must be a positive number.")
        if isinstance(bks_value, float) and not bks_value.is_integer():
            msg = (
                "bks_value must be an integer value. This is because "
                "PyVRP internally uses integer costs."
            )
            raise ValueError(msg)

        self.bks_value = bks_value

    def __call__(self, best_cost: float) -> bool:
        return best_cost <= self.bks_value
