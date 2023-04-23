#include "Measure.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(_Measure, m)
{
#ifdef INT_PRECISION
    m.attr("has_integer_precision") = true;
#else
    m.attr("has_integer_precision") = false;
#endif

    py::class_<distance_type>(m, "distance_type")
        .def(py::init<int>())
        .def(py::init<double>())
        .def("__int__",
             [](distance_type const dist) { return static_cast<int>(dist); })
        .def("__float__", [](distance_type const dist) {
            return static_cast<double>(dist);
        });

    py::class_<duration_type>(m, "duration_type")
        .def(py::init<int>())
        .def(py::init<double>())
        .def("__int__",
             [](duration_type const dur) { return static_cast<int>(dur); })
        .def("__float__",
             [](duration_type const dur) { return static_cast<double>(dur); });

    py::class_<cost_type>(m, "cost_type")
        .def(py::init<int>())
        .def(py::init<double>())
        .def("__int__",
             [](cost_type const cost) { return static_cast<int>(cost); })
        .def("__float__",
             [](cost_type const cost) { return static_cast<double>(cost); });
}
