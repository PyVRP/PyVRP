#include "XorShift128.h"
#include "bindings.h"

namespace py = pybind11;

void bind_XorShift128(py::module_ &m)
{
    py::class_<XorShift128>(m, "XorShift128")
        .def(py::init<int>(), py::arg("seed"));
}
