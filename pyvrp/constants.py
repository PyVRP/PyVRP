MAX_USER_VALUE = 1 << 48
"""
Maximum input value the user may provide. Any bigger value issues a warning
about possible scaling issues.
"""

MAX_VALUE = 1 << 52
"""
The default maximum value that is used internally when an edge is missing in
the input data. 
"""
