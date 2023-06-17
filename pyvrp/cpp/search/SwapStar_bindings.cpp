#include "SwapStar.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_SwapStar, m)
{
    py::class_<LocalSearchOperator<Route>>(
        m, "RouteOperator", py::module_local());

    py::class_<SwapStar, LocalSearchOperator<Route>>(m, "SwapStar")
        .def(py::init<ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
}
