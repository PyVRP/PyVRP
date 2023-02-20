#include "LocalSearch.h"

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(LocalSearch, m)
{
    py::class_<LocalSearchParams>(m, "LocalSearchParams")
        .def(py::init<>());

    py::class_<LocalSearch>(m, "LocalSearch")
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      XorShift128 &,
                      LocalSearchParams>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"),
             py::arg("params"))
        .def(py::init<ProblemData &, PenaltyManager &, XorShift128 &>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"))
        .def("add_node_operator", &LocalSearch::addNodeOperator, py::arg("op"))
        .def(
            "add_route_operator", &LocalSearch::addRouteOperator, py::arg("op"))
        .def("set_neighbours", &LocalSearch::setNeighbours, py::arg("neighbours"))
        .def("get_neighbours", &LocalSearch::getNeighbours)
        .def("search", &LocalSearch::search, py::arg("individual"))
        .def("intensify", &LocalSearch::intensify, py::arg("individual"));
}
