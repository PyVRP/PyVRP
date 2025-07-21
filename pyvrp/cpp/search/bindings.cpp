#include "bindings.h"
#include "ChangeVehicleType.h"
#include "Exchange.h"
#include "LocalSearch.h"
#include "NeighbourRemoval.h"
#include "OptionalInsert.h"
#include "PerturbationOperator.h"
#include "RelocateWithDepot.h"
#include "Route.h"
#include "SwapRoutes.h"
#include "SwapStar.h"
#include "SwapTails.h"
#include "primitives.h"
#include "search_docs.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>

namespace py = pybind11;

using pyvrp::search::ChangeVehicleType;
using pyvrp::search::Exchange;
using pyvrp::search::inplaceCost;
using pyvrp::search::insertCost;
using pyvrp::search::LocalSearch;
using pyvrp::search::NeighbourRemoval;
using pyvrp::search::NodeOperator;
using pyvrp::search::OperatorStatistics;
using pyvrp::search::OptionalInsert;
using pyvrp::search::PerturbationOperator;
using pyvrp::search::RelocateWithDepot;
using pyvrp::search::removeCost;
using pyvrp::search::Route;
using pyvrp::search::RouteOperator;
using pyvrp::search::supports;
using pyvrp::search::SwapRoutes;
using pyvrp::search::SwapStar;
using pyvrp::search::SwapTails;

PYBIND11_MODULE(_search, m)
{
    py::class_<NodeOperator>(m, "NodeOperator");
    py::class_<RouteOperator>(m, "RouteOperator");
    py::class_<PerturbationOperator>(m, "PerturbationOperator");

    py::class_<OperatorStatistics>(
        m, "OperatorStatistics", DOC(pyvrp, search, OperatorStatistics))
        .def_readonly("num_evaluations", &OperatorStatistics::numEvaluations)
        .def_readonly("num_applications", &OperatorStatistics::numApplications);

    py::class_<Exchange<1, 0>, NodeOperator>(
        m, "Exchange10", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &Exchange<1, 0>::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &Exchange<1, 0>::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &Exchange<1, 0>::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<Exchange<1, 0>>, py::arg("data"));

    py::class_<Exchange<2, 0>, NodeOperator>(
        m, "Exchange20", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &Exchange<2, 0>::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &Exchange<2, 0>::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &Exchange<2, 0>::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<Exchange<2, 0>>, py::arg("data"));

    py::class_<Exchange<3, 0>, NodeOperator>(
        m, "Exchange30", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &Exchange<3, 0>::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &Exchange<3, 0>::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &Exchange<3, 0>::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<Exchange<3, 0>>, py::arg("data"));

    py::class_<Exchange<1, 1>, NodeOperator>(
        m, "Exchange11", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &Exchange<1, 1>::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &Exchange<1, 1>::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &Exchange<1, 1>::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<Exchange<1, 1>>, py::arg("data"));

    py::class_<Exchange<2, 1>, NodeOperator>(
        m, "Exchange21", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &Exchange<2, 1>::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &Exchange<2, 1>::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &Exchange<2, 1>::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<Exchange<2, 1>>, py::arg("data"));

    py::class_<Exchange<3, 1>, NodeOperator>(
        m, "Exchange31", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &Exchange<3, 1>::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &Exchange<3, 1>::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &Exchange<3, 1>::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<Exchange<3, 1>>, py::arg("data"));

    py::class_<Exchange<2, 2>, NodeOperator>(
        m, "Exchange22", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &Exchange<2, 2>::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &Exchange<2, 2>::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &Exchange<2, 2>::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<Exchange<2, 2>>, py::arg("data"));

    py::class_<Exchange<3, 2>, NodeOperator>(
        m, "Exchange32", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &Exchange<3, 2>::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &Exchange<3, 2>::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &Exchange<3, 2>::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<Exchange<3, 2>>, py::arg("data"));

    py::class_<Exchange<3, 3>, NodeOperator>(
        m, "Exchange33", DOC(pyvrp, search, Exchange))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &Exchange<3, 3>::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &Exchange<3, 3>::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &Exchange<3, 3>::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<Exchange<3, 3>>, py::arg("data"));

    py::class_<SwapRoutes, RouteOperator>(
        m, "SwapRoutes", DOC(pyvrp, search, SwapRoutes))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &SwapRoutes::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &SwapRoutes::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &SwapRoutes::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<SwapRoutes>, py::arg("data"));

    py::class_<SwapStar, RouteOperator>(
        m, "SwapStar", DOC(pyvrp, search, SwapStar))
        .def(py::init<pyvrp::ProblemData const &, double>(),
             py::arg("data"),
             py::arg("overlap_tolerance") = 0.05,
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &SwapStar::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &SwapStar::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &SwapStar::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<SwapStar>, py::arg("data"));

    py::class_<SwapTails, NodeOperator>(
        m, "SwapTails", DOC(pyvrp, search, SwapTails))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &SwapTails::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &SwapTails::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &SwapTails::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<SwapTails>, py::arg("data"));

    py::class_<RelocateWithDepot, NodeOperator>(
        m, "RelocateWithDepot", DOC(pyvrp, search, RelocateWithDepot))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &RelocateWithDepot::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &RelocateWithDepot::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &RelocateWithDepot::apply, py::arg("U"), py::arg("V"))
        .def_static("supports", &supports<RelocateWithDepot>, py::arg("data"));

    py::class_<NeighbourRemoval, PerturbationOperator>(
        m, "NeighbourRemoval", DOC(pyvrp, search, NeighbourRemoval))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def("__call__",
             &NeighbourRemoval::operator(),
             py::arg("context"),
             py::call_guard<py::gil_scoped_release>())
        .def_static("supports", &supports<NeighbourRemoval>, py::arg("data"));

    py::class_<ChangeVehicleType, PerturbationOperator>(
        m, "ChangeVehicleType", DOC(pyvrp, search, ChangeVehicleType))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def("__call__",
             &ChangeVehicleType::operator(),
             py::arg("context"),
             py::call_guard<py::gil_scoped_release>())
        .def_static("supports", &supports<ChangeVehicleType>, py::arg("data"));

    py::class_<OptionalInsert, PerturbationOperator>(
        m, "OptionalInsert", DOC(pyvrp, search, OptionalInsert))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def("__call__",
             &OptionalInsert::operator(),
             py::arg("context"),
             py::call_guard<py::gil_scoped_release>())
        .def_static("supports", &supports<OptionalInsert>, py::arg("data"));

    py::class_<LocalSearch::Statistics>(
        m, "LocalSearchStatistics", DOC(pyvrp, search, LocalSearch, Statistics))
        .def_readonly("num_moves", &LocalSearch::Statistics::numMoves)
        .def_readonly("num_improving", &LocalSearch::Statistics::numImproving)
        .def_readonly("num_updates", &LocalSearch::Statistics::numUpdates);

    py::class_<LocalSearch>(m, "LocalSearch")
        .def(py::init<pyvrp::ProblemData const &,
                      std::vector<std::vector<size_t>>>(),
             py::arg("data"),
             py::arg("neighbours"),
             py::keep_alive<1, 2>())  // keep data alive until LS is freed
        .def_property("neighbours",
                      &LocalSearch::neighbours,
                      &LocalSearch::setNeighbours,
                      py::return_value_policy::reference_internal)
        .def_property("num_perturbations",
                      &LocalSearch::numPerturbations,
                      &LocalSearch::setNumPerturbations)
        .def_property_readonly("statistics", &LocalSearch::statistics)
        .def_property_readonly("node_operators",
                               &LocalSearch::nodeOperators,
                               py::return_value_policy::reference_internal)
        .def_property_readonly("route_operators",
                               &LocalSearch::routeOperators,
                               py::return_value_policy::reference_internal)
        .def_property_readonly("perturbation_operators",
                               &LocalSearch::perturbationOperators,
                               py::return_value_policy::reference_internal)
        .def("add_node_operator",
             &LocalSearch::addNodeOperator,
             py::arg("op"),
             py::keep_alive<1, 2>())
        .def("add_route_operator",
             &LocalSearch::addRouteOperator,
             py::arg("op"),
             py::keep_alive<1, 2>())
        .def("add_perturbation_operator",
             &LocalSearch::addPerturbationOperator,
             py::arg("op"),
             py::keep_alive<1, 2>())
        .def("__call__",
             &LocalSearch::operator(),
             py::arg("solution"),
             py::arg("cost_evaluator"),
             py::call_guard<py::gil_scoped_release>())
        .def("search",
             py::overload_cast<pyvrp::Solution const &,
                               pyvrp::CostEvaluator const &>(
                 &LocalSearch::search),
             py::arg("solution"),
             py::arg("cost_evaluator"),
             py::call_guard<py::gil_scoped_release>())
        .def("intensify",
             py::overload_cast<pyvrp::Solution const &,
                               pyvrp::CostEvaluator const &>(
                 &LocalSearch::intensify),
             py::arg("solution"),
             py::arg("cost_evaluator"),
             py::call_guard<py::gil_scoped_release>())
        .def("perturb",
             py::overload_cast<pyvrp::Solution const &,
                               pyvrp::CostEvaluator const &>(
                 &LocalSearch::perturb),
             py::arg("solution"),
             py::arg("cost_evaluator"),
             py::call_guard<py::gil_scoped_release>())

        .def("shuffle", &LocalSearch::shuffle, py::arg("rng"))
        .def("set_num_perturbations",
             &LocalSearch::setNumPerturbations,
             py::arg("num_perturb"));

    py::class_<Route>(m, "Route", DOC(pyvrp, search, Route))
        .def(py::init<pyvrp::ProblemData const &, size_t, size_t>(),
             py::arg("data"),
             py::arg("idx"),
             py::arg("vehicle_type"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("idx", &Route::idx)
        .def_property_readonly("vehicle_type", &Route::vehicleType)
        .def("num_clients", &Route::numClients)
        .def("num_depots", &Route::numDepots)
        .def("num_trips", &Route::numTrips)
        .def("max_trips", &Route::maxTrips)
        .def("__delitem__", &Route::remove, py::arg("idx"))
        .def(
            "__getitem__",
            [](Route const &route, int idx)
            {
                // int so we also support negative offsets from the end.
                idx = idx < 0 ? route.size() + idx : idx;
                if (idx < 0 || static_cast<size_t>(idx) >= route.size())
                    throw py::index_error();
                return route[idx];
            },
            py::arg("idx"),
            py::return_value_policy::reference_internal)
        .def(
            "__iter__",
            [](Route const &route)
            { return py::make_iterator(route.begin(), route.end()); },
            py::return_value_policy::reference_internal)
        .def("__len__", &Route::size)
        .def("__str__",
             [](Route const &route)
             {
                 std::stringstream stream;
                 stream << route;
                 return stream.str();
             })
        .def("is_feasible", &Route::isFeasible)
        .def("has_excess_load", &Route::hasExcessLoad)
        .def("has_excess_distance", &Route::hasExcessDistance)
        .def("has_time_warp", &Route::hasTimeWarp)
        .def("capacity",
             &Route::capacity,
             py::return_value_policy::reference_internal)
        .def("start_depot", &Route::startDepot)
        .def("end_depot", &Route::endDepot)
        .def("fixed_vehicle_cost", &Route::fixedVehicleCost)
        .def("load", &Route::load, py::return_value_policy::reference_internal)
        .def("excess_load",
             &Route::excessLoad,
             py::return_value_policy::reference_internal)
        .def("excess_distance", &Route::excessDistance)
        .def("distance", &Route::distance)
        .def("distance_cost", &Route::distanceCost)
        .def("unit_distance_cost", &Route::unitDistanceCost)
        .def("has_distance_cost", &Route::hasDistanceCost)
        .def("duration", &Route::duration)
        .def("duration_cost", &Route::durationCost)
        .def("unit_duration_cost", &Route::unitDurationCost)
        .def("has_duration_cost", &Route::hasDurationCost)
        .def("max_duration", &Route::maxDuration)
        .def("max_distance", &Route::maxDistance)
        .def("time_warp", &Route::timeWarp)
        .def("profile", &Route::profile)
        .def(
            "dist_at",
            [](Route const &route, size_t idx, size_t profile)
            { return route.at(idx).distance(profile); },
            py::arg("idx"),
            py::arg("profile") = 0)
        .def(
            "dist_between",
            [](Route const &route, size_t start, size_t end, size_t profile)
            { return route.between(start, end).distance(profile); },
            py::arg("start"),
            py::arg("end"),
            py::arg("profile") = 0)
        .def(
            "dist_after",
            [](Route const &route, size_t start)
            { return route.after(start).distance(route.profile()); },
            py::arg("start"))
        .def(
            "dist_before",
            [](Route const &route, size_t end)
            { return route.before(end).distance(route.profile()); },
            py::arg("end"))
        .def(
            "load_at",
            [](Route const &route, size_t idx, size_t dimension)
            { return route.at(idx).load(dimension); },
            py::arg("idx"),
            py::arg("dimension") = 0)
        .def(
            "load_between",
            [](Route const &route, size_t start, size_t end, size_t dimension)
            { return route.between(start, end).load(dimension); },
            py::arg("start"),
            py::arg("end"),
            py::arg("dimension") = 0)
        .def(
            "load_after",
            [](Route const &route, size_t start, size_t dimension)
            { return route.after(start).load(dimension); },
            py::arg("start"),
            py::arg("dimension") = 0)
        .def(
            "load_before",
            [](Route const &route, size_t end, size_t dimension)
            { return route.before(end).load(dimension); },
            py::arg("end"),
            py::arg("dimension") = 0)
        .def(
            "duration_at",
            [](Route const &route, size_t idx, size_t profile)
            { return route.at(idx).duration(profile); },
            py::arg("idx"),
            py::arg("profile") = 0)
        .def(
            "duration_between",
            [](Route const &route, size_t start, size_t end, size_t profile)
            { return route.between(start, end).duration(profile); },
            py::arg("start"),
            py::arg("end"),
            py::arg("profile") = 0)
        .def(
            "duration_after",
            [](Route const &route, size_t start)
            { return route.after(start).duration(route.profile()); },
            py::arg("start"))
        .def(
            "duration_before",
            [](Route const &route, size_t end)
            { return route.before(end).duration(route.profile()); },
            py::arg("end"))
        .def("centroid", &Route::centroid)
        .def("overlaps_with",
             &Route::overlapsWith,
             py::arg("other"),
             py::arg("tolerance"))
        .def("append",
             &Route::push_back,
             py::arg("node"),
             py::keep_alive<1, 2>(),  // keep node alive
             py::keep_alive<2, 1>())  // keep route alive
        .def("clear", &Route::clear)
        .def(
            "insert",
            [](Route &route, size_t idx, Route::Node *node)
            { route.insert(idx, node); },
            py::arg("idx"),
            py::arg("node"),
            py::keep_alive<1, 3>(),  // keep node alive
            py::keep_alive<3, 1>())  // keep route alive
        .def_static("swap", &Route::swap, py::arg("first"), py::arg("second"))
        .def("update", &Route::update);

    py::class_<Route::Node>(m, "Node", DOC(pyvrp, search, Route, Node))
        .def(py::init<size_t>(), py::arg("loc"))
        .def_property_readonly("client", &Route::Node::client)
        .def_property_readonly("idx", &Route::Node::idx)
        .def_property_readonly("trip", &Route::Node::trip)
        .def_property_readonly("route", &Route::Node::route)
        .def("is_depot", &Route::Node::isDepot)
        .def("is_start_depot", &Route::Node::isStartDepot)
        .def("is_end_depot", &Route::Node::isEndDepot)
        .def("is_reload_depot", &Route::Node::isReloadDepot)
        .def("__str__",
             [](Route::Node const &node)
             {
                 std::stringstream stream;
                 stream << node;
                 return stream.str();
             });

    m.def("insert_cost",
          &insertCost,
          py::arg("U"),
          py::arg("V"),
          py::arg("data"),
          py::arg("cost_evaluator"),
          DOC(pyvrp, search, insertCost));

    m.def("inplace_cost",
          &inplaceCost,
          py::arg("U"),
          py::arg("V"),
          py::arg("data"),
          py::arg("cost_evaluator"),
          DOC(pyvrp, search, inplaceCost));

    m.def("remove_cost",
          &removeCost,
          py::arg("U"),
          py::arg("data"),
          py::arg("cost_evaluator"),
          DOC(pyvrp, search, removeCost));
}
