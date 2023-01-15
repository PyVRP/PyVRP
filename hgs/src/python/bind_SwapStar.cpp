#include "SwapStar.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_SwapStar(py::module_ &m)
{
    py::class_<SwapStar, LocalSearchOperator<Route>>(m, "SwapStar")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));
}
