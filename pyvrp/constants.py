import sys

MAX_VALUE = 1 << 44
"""
The largest value that can be passed for any element of the input distance or
duration matrices (including missing values). Passing larger values warns about
possible overflow due to scaling issues.
"""

MAX_SIZE = 2 * sys.maxsize + 1
"""
Maximum unsigned size (the maximum value of a ``size_t``) on this platform.
"""
