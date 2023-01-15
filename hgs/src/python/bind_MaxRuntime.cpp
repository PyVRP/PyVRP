#include "MaxRuntime.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_MaxRuntime(py::module_ &m)
{
    py::class_<MaxRuntime, StoppingCriterion>(m, "MaxRuntime")
        .def(py::init<double>(), py::arg("max_runtime"))
        .def("__call__", &MaxRuntime::operator(), py::arg("best_cost"));
}
