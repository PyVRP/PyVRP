#include "MoveTwoClientsReversed.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_MoveTwoClientsReversed, m)
{
    py::class_<LocalSearchOperator<Node>>(
        m, "NodeOperator", py::module_local());

    py::class_<MoveTwoClientsReversed, LocalSearchOperator<Node>>(
        m, "MoveTwoClientsReversed")
        .def(py::init<ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
}
