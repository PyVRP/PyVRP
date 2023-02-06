# CPP

This folder contains all C++ extension classes and functions. These extensions
are ordered in the way they will appear in the `pyvrp` package: everything in
this top-level folder will be made available in the top-level `pyvrp` package,
whereas for example everything in `cpp/educate` will be present in 
`pyvrp.educate` after installation (and so on for the other folders).

We use `pybind11` to generate the Python bindings for the C++ codebase. These
bindings are generated in the `X_bindings.cpp` source files of each object `X`
that we want to expose to Python. 
