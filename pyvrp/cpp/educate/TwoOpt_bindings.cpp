#include "TwoOpt.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(TwoOpt, m)
{
    py::class_<LocalSearchOperator<Node>>(
        m, "NodeOperator", py::module_local());

    py::class_<TwoOpt, LocalSearchOperator<Node>>(m, "TwoOpt")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));
}
