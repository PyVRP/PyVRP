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
          py::arg("rtol") = 1e-6,
          py::arg("atol") = 1e-8);
    m.def("equal_int",
          &equal<int>,
          py::arg("a"),
          py::arg("b"),
          py::arg("rtol") = 1e-6,
          py::arg("atol") = 1e-6);
    m.def("smaller_float",
          &smaller<double>,
          py::arg("a"),
          py::arg("b"),
          py::arg("rtol") = 1e-6,
          py::arg("atol") = 1e-8);
    m.def("smaller_int",
          &smaller<int>,
          py::arg("a"),
          py::arg("b"),
          py::arg("rtol") = 1e-6,
          py::arg("atol") = 1e-6);
    m.def("greater_float",
          &greater<double>,
          py::arg("a"),
          py::arg("b"),
          py::arg("rtol") = 1e-6,
          py::arg("atol") = 1e-8);
    m.def("greater_int",
          &greater<int>,
          py::arg("a"),
          py::arg("b"),
          py::arg("rtol") = 1e-6,
          py::arg("atol") = 1e-6);
}
