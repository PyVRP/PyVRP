#include "TimedNoImprovement.h"
#include "bindings.h"

namespace py = pybind11;

void bind_TimedNoImprovement(py::module_ &m)
{
    py::class_<TimedNoImprovement, StoppingCriterion>(m, "TimedNoImprovement")
        .def(py::init<size_t, double>(),
             py::arg("max_iterations"),
             py::arg("max_runtime"))
        .def("__call__", &TimedNoImprovement::operator(), py::arg("best_cost"));
}
