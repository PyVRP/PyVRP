#include "LocalSearch.h"

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(_LocalSearch, m)
{
    py::class_<LocalSearch>(m, "LocalSearch")
        .def(py::init<ProblemData const &, std::vector<std::vector<int>>>(),
             py::arg("data"),
             py::arg("neighbours"),
             py::keep_alive<1, 2>())  // keep data alive until LS is freed
        .def("add_node_operator",
             &LocalSearch::addNodeOperator,
             py::arg("op"),
             py::keep_alive<1, 2>())
        .def("add_route_operator",
             &LocalSearch::addRouteOperator,
             py::arg("op"),
             py::keep_alive<1, 2>())
        .def("set_neighbours",
             &LocalSearch::setNeighbours,
             py::arg("neighbours"))
        .def("get_neighbours",
             &LocalSearch::getNeighbours,
             py::return_value_policy::reference_internal)
        .def("search",
             &LocalSearch::search,
             py::arg("solution"),
             py::arg("cost_evaluator"))
        .def("intensify",
             &LocalSearch::intensify,
             py::arg("solution"),
             py::arg("cost_evaluator"),
             py::arg("overlap_tolerance_degrees") = 0)
        .def("shuffle", &LocalSearch::shuffle, py::arg("rng"));
}
