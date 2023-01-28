#include "XorShift128.h"
#include "bindings.h"

namespace py = pybind11;

void bind_XorShift128(py::module_ &m)
{
    py::class_<XorShift128>(m, "XorShift128")
        .def(py::init<int>(), py::arg("seed"))
        .def("min", &XorShift128::min)
        .def("max", &XorShift128::max)
        .def("__call__", &XorShift128::operator())
        .def("randint", &XorShift128::randint<int>, py::arg("high"));
}
