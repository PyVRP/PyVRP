MAX_VALUE = 1 << 52
"""
The default maximum value that is used internally when an edge is missing in
the input data. This is the largest value that can be passed for any element of
the input distance or duration matrices. Passing larger values warns about 
possible overflow due to scaling issues.
"""
