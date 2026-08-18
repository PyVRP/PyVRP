import sys

MAX_VALUE = 1 << 44
"""
The largest value that can be passed for any element of the input distance or
duration matrices (including missing values). Passing larger values warns about
possible overflow due to scaling issues.
"""

INT_MAX = sys.maxsize
UINT_MAX = 2 * INT_MAX + 1
