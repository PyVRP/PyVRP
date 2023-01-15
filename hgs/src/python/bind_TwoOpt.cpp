#include "TwoOpt.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_TwoOpt(py::module_ &m)
{
    py::class_<TwoOpt, LocalSearchOperator<Node>>(m, "TwoOpt")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));
}
