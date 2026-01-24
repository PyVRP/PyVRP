# This file is part of the PyVRP project (https://github.com/PyVRP/PyVRP), and
# licensed under the terms of the MIT license.

MAX_VALUE = 1 << 44
"""
The largest value that can be passed for any element of the input distance or
duration matrices (including missing values). Passing larger values warns about
possible overflow due to scaling issues.
"""
