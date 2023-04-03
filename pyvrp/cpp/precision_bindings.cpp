#include "precision.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_precision, m)
{
#ifdef INT_PRECISION
    m.attr("PRECISION") = "int",
#else
    m.attr("PRECISION") = "double",
#endif
    m.def("equal_float",
          &equal<double>,
          py::arg("a"),
          py::arg("b"),
          py::arg("tol") = 1e-6);
    m.def("equal_int",
          &equal<int>,
          py::arg("a"),
          py::arg("b"),
          py::arg("tol") = 1e-6);
}
