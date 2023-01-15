#include "RelocateStar.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_RelocateStar(py::module_ &m)
{
    py::class_<RelocateStar, LocalSearchOperator<Route>>(m, "RelocateStar")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));
}
