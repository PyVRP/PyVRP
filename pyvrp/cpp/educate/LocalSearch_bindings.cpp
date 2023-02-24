#include "LocalSearch.h"

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(LocalSearch, m)
{
    py::class_<LocalSearch>(m, "LocalSearch")
        .def(py::init<ProblemData &, PenaltyManager &, XorShift128 &>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"))
        .def("add_node_operator", &LocalSearch::addNodeOperator, py::arg("op"))
        .def(
            "add_route_operator", &LocalSearch::addRouteOperator, py::arg("op"))
        .def("set_neighbours",
             &LocalSearch::setNeighbours,
             py::arg("neighbours"))
        .def("get_neighbours", &LocalSearch::getNeighbours)
        .def("search", &LocalSearch::search, py::arg("individual"))
        .def("intensify",
             &LocalSearch::intensify,
             py::arg("individual"),
             py::arg("overlap_tolerance_degrees") = 0);
}
