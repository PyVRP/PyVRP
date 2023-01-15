#include "NoImprovement.h"
#include "bindings.h"

namespace py = pybind11;

void bind_NoImprovement(py::module_ &m)
{
    py::class_<NoImprovement, StoppingCriterion>(m, "NoImprovement")
        .def(py::init<size_t>(), py::arg("max_iterations"))
        .def("__call__", &NoImprovement::operator(), py::arg("best_cost"));
}
