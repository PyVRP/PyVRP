# Native extensions

This folder contains all C++ extension classes and functions. These extensions
are ordered in the way they will appear in the `pyvrp` package: everything in
this top-level folder will be made available in the top-level `pyvrp` package,
whereas for example everything in `cpp/search` will be present in 
`pyvrp.search` after installation (and so on for the other folders).

> When building on our implementation, please refrain from defining symbols
> starting with the `PYVRP_` prefix, or in the `pyvrp` namespace. PyVRP
> reserves these for internal use.

## Bindings

We use `pybind11` to generate Python bindings for the C++ codebase. These
bindings are generated from the `bindings.cpp` source files of each folder. 
Each `bindings.cpp` file is compiled into a single extension module named after
the installation folder, prefixed with an underscore.

The bindings depend on header files ending in `_docs.h`. Those header files
contain docstrings that are automatically extracted from the other header files
present in this directory. The documentation headers are generated automatically
by Meson during compilation.

## Logging

We use `spdlog` for logging in the C++ codebase. Logging is done using the
wrapper macros provided in `logging.h`. DEBUG logs are present only in debug
builds, but everything above that is present in release builds, too. The logs
are shared with Python via the bindings, and further processing can be done
there using the familiar `logging` interface.

Our loggers nest under the top-level `"pyvrp"` logger, and match the folder
structure (e.g., `pyvrp/search` uses the `"pyvrp.search"` logger).
