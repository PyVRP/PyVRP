#include "TimedNoImprovement.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_TimedNoImprovement(py::module_ &m)
{
    py::class_<TimedNoImprovement, StoppingCriterion>(m, "TimedNoImprovement")
        .def(py::init<size_t, double>(),
             py::arg("max_iterations"),
             py::arg("max_runtime"))
        .def("__call__", &TimedNoImprovement::operator(), py::arg("best_cost"));
}
