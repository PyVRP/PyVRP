#include "XorShift128.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_XorShift128, m)
{
    py::class_<XorShift128>(m, "XorShift128")
        .def(py::init<uint32_t>(), py::arg("seed"))
        .def("min", &XorShift128::min)
        .def("max", &XorShift128::max)
        .def("__call__", &XorShift128::operator())
        .def("rand", &XorShift128::rand<double>)
        .def("randint", &XorShift128::randint<int>, py::arg("high"));
}
