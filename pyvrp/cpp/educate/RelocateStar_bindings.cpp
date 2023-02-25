#include "RelocateStar.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_RelocateStar, m)
{
    py::class_<LocalSearchOperator<Route>>(
        m, "RouteOperator", py::module_local());

    py::class_<RelocateStar, LocalSearchOperator<Route>>(m, "RelocateStar")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));
}
