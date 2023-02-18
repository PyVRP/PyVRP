#include "LocalSearch.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(LocalSearch, m)
{
    py::class_<LocalSearchParams>(m, "LocalSearchParams")
        .def(py::init<size_t, size_t, size_t, size_t>(),
             py::arg("weight_wait_time") = 18,
             py::arg("weight_time_warp") = 20,
             py::arg("nb_granular") = 34)
        .def_readonly("weight_wait_time", &LocalSearchParams::weightWaitTime)
        .def_readonly("weight_time_warp", &LocalSearchParams::weightTimeWarp)
        .def_readonly("nb_granular", &LocalSearchParams::nbGranular);

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
        .def("search", &LocalSearch::search, py::arg("individual"))
        .def("intensify", &LocalSearch::intensify, py::arg("individual"));
}
