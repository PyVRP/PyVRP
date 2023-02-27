from pyvrp._Individual import Individual

def broken_pairs_distance(first: Individual, second: Individual) -> float:
    """
    Computes the symmetric broken pairs distance between the given two
    individuals. This function determines whether each client in the problem
    shares neighbours between the first and second solution. If not, the
    client is part of a 'broken pair': a link that is part of one solution,
    but not of the other.

    Parameters
    ----------
    first
        First individual.
    second
        Second individual.

    Returns
    -------
    float
        A value in [0, 1] that indicates the percentage of broken links between
        the two individuals. A value near one suggests the solutions are
        maximally diverse, a value of zero indicates they are the same.
    """
