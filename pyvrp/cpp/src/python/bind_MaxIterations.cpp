#include "MaxIterations.h"
#include "bindings.h"

namespace py = pybind11;

void bind_MaxIterations(py::module_ &m)
{
    py::class_<MaxIterations, StoppingCriterion>(m, "MaxIterations")
        .def(py::init<size_t>(), py::arg("max_iterations"))
        .def("__call__", &MaxIterations::operator(), py::arg("best_cost"));
}
