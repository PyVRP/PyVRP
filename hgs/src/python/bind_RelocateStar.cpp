#include "RelocateStar.h"
#include "bindings.h"

namespace py = pybind11;

void bind_RelocateStar(py::module_ &m)
{
    py::class_<RelocateStar, LocalSearchOperator<Route>>(m, "RelocateStar")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));
}
