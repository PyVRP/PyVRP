#include "precision.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_precision, m)
{
    m.def("greater_float",
          &greater<double>,
          py::arg("a"),
          py::arg("b"),
          py::arg("tol") = 1e-6);
    m.def("greater_int",
          &greater<int>,
          py::arg("a"),
          py::arg("b"),
          py::arg("tol") = 1e-6);

    m.def("greater_equal_float",
          &greater_equal<double>,
          py::arg("a"),
          py::arg("b"),
          py::arg("tol") = 1e-6);
    m.def("greater_equal_int",
          &greater_equal<int>,
          py::arg("a"),
          py::arg("b"),
          py::arg("tol") = 1e-6);

    m.def("less_float",
          &less<double>,
          py::arg("a"),
          py::arg("b"),
          py::arg("tol") = 1e-6);
    m.def("less_int",
          &less<int>,
          py::arg("a"),
          py::arg("b"),
          py::arg("tol") = 1e-6);

    m.def("less_equal_float",
          &less_equal<double>,
          py::arg("a"),
          py::arg("b"),
          py::arg("tol") = 1e-6);
    m.def("less_equal_int",
          &less_equal<int>,
          py::arg("a"),
          py::arg("b"),
          py::arg("tol") = 1e-6);

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
