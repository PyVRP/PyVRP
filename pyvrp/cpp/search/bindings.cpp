#include "Exchange.h"
#include "LocalSearch.h"
#include "MoveTwoClientsReversed.h"
#include "RelocateStar.h"
#include "SwapStar.h"
#include "TwoOpt.h"
#include "search_docs.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using pyvrp::search::Exchange;
using pyvrp::search::LocalSearch;
using pyvrp::search::LocalSearchOperator;
using pyvrp::search::MoveTwoClientsReversed;
using pyvrp::search::RelocateStar;
using pyvrp::search::SwapStar;
using pyvrp::search::TwoOpt;

PYBIND11_MODULE(_search, m)
{
    using NodeOp = LocalSearchOperator<pyvrp::search::Route::Node>;
    using RouteOp = LocalSearchOperator<pyvrp::search::Route>;

    py::class_<NodeOp>(m, "NodeOperator");
    py::class_<RouteOp>(m, "RouteOperator");

    py::class_<Exchange<1, 0>, NodeOp>(
        m, "Exchange10", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>());  // keep data alive

    py::class_<Exchange<2, 0>, NodeOp>(
        m, "Exchange20", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>());  // keep data alive

    py::class_<Exchange<3, 0>, NodeOp>(
        m, "Exchange30", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>());  // keep data alive

    py::class_<Exchange<1, 1>, NodeOp>(
        m, "Exchange11", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>());  // keep data alive

    py::class_<Exchange<2, 1>, NodeOp>(
        m, "Exchange21", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>());  // keep data alive

    py::class_<Exchange<3, 1>, NodeOp>(
        m, "Exchange31", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>());  // keep data alive

    py::class_<Exchange<2, 2>, NodeOp>(
        m, "Exchange22", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>());  // keep data alive

    py::class_<Exchange<3, 2>, NodeOp>(
        m, "Exchange32", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>());  // keep data alive

    py::class_<Exchange<3, 3>, NodeOp>(
        m, "Exchange33", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>());  // keep data alive

    py::class_<LocalSearch>(m, "LocalSearch")
        .def(py::init<pyvrp::ProblemData const &,
                      std::vector<std::vector<int>>>(),
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
             py::arg("overlap_tolerance") = 0.05)
        .def("shuffle", &LocalSearch::shuffle, py::arg("rng"));

    py::class_<MoveTwoClientsReversed, NodeOp>(m, "MoveTwoClientsReversed")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<RelocateStar, RouteOp>(m, "RelocateStar")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<SwapStar, RouteOp>(m, "SwapStar", DOC(pyvrp, search, SwapStar))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<TwoOpt, NodeOp>(m, "TwoOpt")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
}
