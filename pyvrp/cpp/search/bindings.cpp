#include "Exchange.h"
#include "LocalSearch.h"
#include "MoveTwoClientsReversed.h"
#include "RelocateStar.h"
#include "SwapStar.h"
#include "TwoOpt.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace pyvrp::search;

PYBIND11_MODULE(_search, m)
{
    py::class_<LocalSearchOperator<Node>>(m, "NodeOperator");
    py::class_<LocalSearchOperator<Route>>(m, "RouteOperator");

    py::class_<Exchange<1, 0>, LocalSearchOperator<Node>>(m, "Exchange10")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
    py::class_<Exchange<2, 0>, LocalSearchOperator<Node>>(m, "Exchange20")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
    py::class_<Exchange<3, 0>, LocalSearchOperator<Node>>(m, "Exchange30")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
    py::class_<Exchange<1, 1>, LocalSearchOperator<Node>>(m, "Exchange11")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
    py::class_<Exchange<2, 1>, LocalSearchOperator<Node>>(m, "Exchange21")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<Exchange<3, 1>, LocalSearchOperator<Node>>(m, "Exchange31")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<Exchange<2, 2>, LocalSearchOperator<Node>>(m, "Exchange22")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<Exchange<3, 2>, LocalSearchOperator<Node>>(m, "Exchange32")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<Exchange<3, 3>, LocalSearchOperator<Node>>(m, "Exchange33")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

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
             py::arg("overlap_tolerance_degrees") = 0)
        .def("shuffle", &LocalSearch::shuffle, py::arg("rng"));

    py::class_<MoveTwoClientsReversed, LocalSearchOperator<Node>>(
        m, "MoveTwoClientsReversed")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<RelocateStar, LocalSearchOperator<Route>>(m, "RelocateStar")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<SwapStar, LocalSearchOperator<Route>>(m, "SwapStar")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<TwoOpt, LocalSearchOperator<Node>>(m, "TwoOpt")
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
}
