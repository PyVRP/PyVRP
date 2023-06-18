#include "TwoOpt.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_TwoOpt, m)
{
    py::class_<LocalSearchOperator<Node>>(
        m, "NodeOperator", py::module_local());

    py::class_<TwoOpt, LocalSearchOperator<Node>>(m, "TwoOpt")
        .def(py::init<ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
}
