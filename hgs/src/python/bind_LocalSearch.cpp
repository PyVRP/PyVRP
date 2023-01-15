#include "LocalSearch.h"
#include "bindings.h"

namespace py = pybind11;

void bind_LocalSearch(py::module_ &m)
{
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
        .def("search", &LocalSearch::search, py::arg("indiv"))
        .def("intensify", &LocalSearch::intensify, py::arg("indiv"));
}
