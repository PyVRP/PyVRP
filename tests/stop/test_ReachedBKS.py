import pytest
from pyvrp import solve
from pyvrp.Result import Result
from pyvrp.stop import ReachedBKS

def test_zero_bks_value_raises_error():
    """
    Test that zero BKS value raises ValueError.
    """
    with pytest.raises(ValueError):
        ReachedBKS(0)

def test_throw_error_on_float_bks_value():
    """
    Test that a non-integer float BKS value raises ValueError.
    """
    with pytest.raises(ValueError):
        ReachedBKS(123.45) # type: ignore

def test_solve_on_trivial_instance(ok_small):
    """
    Test that the stopping criterion works on a trivial instance.
    """
    bks_value = 9155  # Assume we know the best known solution value
    stop_criterion = ReachedBKS(bks_value)

    res: Result = solve(ok_small, stop=stop_criterion, seed=0)
    
    # The best found solution is found in 0 iterations
    assert res.num_iterations == 0

@pytest.mark.slow
def test_solve_on_solomon_instance_dimacs(rc208):
    """
    Test that the stopping criterion works on a Solomon instance.
    NOTE: We use DIMACS rounding here so that the BKS value is an integer.
    """
    bks_value = 7761  # Assume we know the best known solution value
    stop_criterion = ReachedBKS(bks_value)

    res: Result = solve(rc208, stop=stop_criterion, seed=0)
    
    assert res.num_iterations == 1697

@pytest.mark.slow
def test_solve_on_solomon_instance_original(rc208_original):
    """
    Test that the stopping criterion works on the original Solomon instance.
    NOTE: This instance has floating-point costs, so we use the exact BKS
    value.
    """
    bks_value = 754  # Assume we know the best known solution value
    stop_criterion = ReachedBKS(bks_value)

    res: Result = solve(rc208_original, stop=stop_criterion, seed=0)
    
    assert res.num_iterations == 4705
