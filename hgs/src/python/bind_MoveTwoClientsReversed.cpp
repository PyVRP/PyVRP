#include "MoveTwoClientsReversed.h"
#include "bindings.h"

namespace py = pybind11;

using NodeOp = LocalSearchOperator<Node>;

void bind_MoveTwoClientsReversed(py::module_ &m)
{
    py::class_<MoveTwoClientsReversed, NodeOp>(m, "MoveTwoClientsReversed")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));
}
