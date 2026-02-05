#include "bindings.h"
#include "Exchange.h"
#include "InsertOptional.h"
#include "LocalSearch.h"
#include "PerturbationManager.h"
#include "RelocateWithDepot.h"
#include "RemoveAdjacentDepot.h"
#include "RemoveOptional.h"
#include "ReplaceOptional.h"
#include "Route.h"
#include "SearchSpace.h"
#include "Solution.h"
#include "SwapTails.h"
#include "search_docs.h"

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>

namespace py = pybind11;

using pyvrp::search::BinaryOperator;
using pyvrp::search::Exchange;
using pyvrp::search::InsertOptional;
using pyvrp::search::LocalSearch;
using pyvrp::search::OperatorStatistics;
using pyvrp::search::PerturbationManager;
using pyvrp::search::PerturbationParams;
using pyvrp::search::RelocateWithDepot;
using pyvrp::search::RemoveAdjacentDepot;
using pyvrp::search::RemoveOptional;
using pyvrp::search::ReplaceOptional;
using pyvrp::search::Route;
using pyvrp::search::SearchSpace;
using pyvrp::search::Solution;
using pyvrp::search::supports;
using pyvrp::search::SwapTails;
using pyvrp::search::UnaryOperator;

PYBIND11_MODULE(_search, m)
{
    py::class_<UnaryOperator>(m, "UnaryOperator");
    py::class_<BinaryOperator>(m, "BinaryOperator");

    py::class_<OperatorStatistics>(
        m, "OperatorStatistics", DOC(pyvrp, search, OperatorStatistics))
        .def_readonly("num_evaluations", &OperatorStatistics::numEvaluations)
        .def_readonly("num_applications", &OperatorStatistics::numApplications);

    py::class_<RemoveAdjacentDepot, UnaryOperator>(
        m, "RemoveAdjacentDepot", DOC(pyvrp, search, RemoveAdjacentDepot))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &RemoveAdjacentDepot::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &RemoveAdjacentDepot::evaluate,
             py::arg("U"),
             py::arg("cost_evaluator"))
        .def("apply", &RemoveAdjacentDepot::apply, py::arg("U"))
        .def("init", &RemoveAdjacentDepot::init, py::arg("solution"))
        .def_static(
            "supports", &supports<RemoveAdjacentDepot>, py::arg("data"));

    py::class_<RemoveOptional, UnaryOperator>(
        m, "RemoveOptional", DOC(pyvrp, search, RemoveOptional))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &RemoveOptional::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &RemoveOptional::evaluate,
             py::arg("U"),
             py::arg("cost_evaluator"))
        .def("apply", &RemoveOptional::apply, py::arg("U"))
        .def("init", &RemoveOptional::init, py::arg("solution"))
        .def_static("supports", &supports<RemoveOptional>, py::arg("data"));

    py::class_<InsertOptional, BinaryOperator>(
        m, "InsertOptional", DOC(pyvrp, search, InsertOptional))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &InsertOptional::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &InsertOptional::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &InsertOptional::apply, py::arg("U"), py::arg("V"))
        .def("init", &InsertOptional::init, py::arg("solution"))
        .def_static("supports", &supports<InsertOptional>, py::arg("data"));

    py::class_<ReplaceOptional, BinaryOperator>(
        m, "ReplaceOptional", DOC(pyvrp, search, ReplaceOptional))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("statistics",
                               &ReplaceOptional::statistics,
                               py::return_value_policy::reference_internal)
        .def("evaluate",
             &ReplaceOptional::evaluate,
             py::arg("U"),
             py::arg("V"),
             py::arg("cost_evaluator"))
        .def("apply", &ReplaceOptional::apply, py::arg("U"), py::arg("V"))
        .def("init", &ReplaceOptional::init, py::arg("solution"))
        .def_static("supports", &supports<ReplaceOptional>, py::arg("data"));

    py::class_<Exchange<1, 0>, BinaryOperator>(
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
        .def("init", &Exchange<1, 0>::init, py::arg("solution"))
        .def_static("supports", &supports<Exchange<1, 0>>, py::arg("data"));

    py::class_<Exchange<2, 0>, BinaryOperator>(
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
        .def("init", &Exchange<2, 0>::init, py::arg("solution"))
        .def_static("supports", &supports<Exchange<2, 0>>, py::arg("data"));

    py::class_<Exchange<3, 0>, BinaryOperator>(
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
        .def("init", &Exchange<3, 0>::init, py::arg("solution"))
        .def_static("supports", &supports<Exchange<3, 0>>, py::arg("data"));

    py::class_<Exchange<1, 1>, BinaryOperator>(
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
        .def("init", &Exchange<1, 1>::init, py::arg("solution"))
        .def_static("supports", &supports<Exchange<1, 1>>, py::arg("data"));

    py::class_<Exchange<2, 1>, BinaryOperator>(
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
        .def("init", &Exchange<2, 1>::init, py::arg("solution"))
        .def_static("supports", &supports<Exchange<2, 1>>, py::arg("data"));

    py::class_<Exchange<3, 1>, BinaryOperator>(
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
        .def("init", &Exchange<3, 1>::init, py::arg("solution"))
        .def_static("supports", &supports<Exchange<3, 1>>, py::arg("data"));

    py::class_<Exchange<2, 2>, BinaryOperator>(
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
        .def("init", &Exchange<2, 2>::init, py::arg("solution"))
        .def_static("supports", &supports<Exchange<2, 2>>, py::arg("data"));

    py::class_<Exchange<3, 2>, BinaryOperator>(
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
        .def("init", &Exchange<3, 2>::init, py::arg("solution"))
        .def_static("supports", &supports<Exchange<3, 2>>, py::arg("data"));

    py::class_<Exchange<3, 3>, BinaryOperator>(
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
        .def("init", &Exchange<3, 3>::init, py::arg("solution"))
        .def_static("supports", &supports<Exchange<3, 3>>, py::arg("data"));

    py::class_<SwapTails, BinaryOperator>(
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
        .def("init", &SwapTails::init, py::arg("solution"))
        .def_static("supports", &supports<SwapTails>, py::arg("data"));

    py::class_<RelocateWithDepot, BinaryOperator>(
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
        .def("init", &RelocateWithDepot::init, py::arg("solution"))
        .def_static("supports", &supports<RelocateWithDepot>, py::arg("data"));

    py::class_<SearchSpace>(m, "SearchSpace", DOC(pyvrp, search, SearchSpace))
        .def(py::init<pyvrp::ProblemData const &,
                      std::vector<std::vector<size_t>>>(),
             py::arg("data"),
             py::arg("neighbours"))
        .def_property("neighbours",
                      &SearchSpace::neighbours,
                      &SearchSpace::setNeighbours,
                      py::return_value_policy::reference_internal)
        .def("neighbours_of",
             &SearchSpace::neighboursOf,
             py::arg("client"),
             DOC(pyvrp, search, SearchSpace, neighboursOf))
        .def("is_promising",
             &SearchSpace::isPromising,
             py::arg("client"),
             DOC(pyvrp, search, SearchSpace, isPromising))
        .def("mark_promising",
             py::overload_cast<size_t>(&SearchSpace::markPromising),
             py::arg("client"),
             DOC(pyvrp, search, SearchSpace, markPromising, 1))
        .def(
            "mark_promising",
            py::overload_cast<Route::Node const *>(&SearchSpace::markPromising),
            py::arg("node"),
            DOC(pyvrp, search, SearchSpace, markPromising, 2))
        .def("mark_all_promising",
             &SearchSpace::markAllPromising,
             DOC(pyvrp, search, SearchSpace, markAllPromising))
        .def("unmark_all_promising",
             &SearchSpace::unmarkAllPromising,
             DOC(pyvrp, search, SearchSpace, unmarkAllPromising))
        .def("client_order",
             &SearchSpace::clientOrder,
             DOC(pyvrp, search, SearchSpace, clientOrder))
        .def("veh_type_order",
             &SearchSpace::vehTypeOrder,
             DOC(pyvrp, search, SearchSpace, vehTypeOrder))
        .def("shuffle",
             &SearchSpace::shuffle,
             py::arg("rng"),
             DOC(pyvrp, search, SearchSpace, shuffle));

    py::class_<PerturbationParams>(
        m, "PerturbationParams", DOC(pyvrp, search, PerturbationParams))
        .def(py::init<size_t, size_t>(),
             py::arg("min_perturbations") = 1,
             py::arg("max_perturbations") = 25)
        .def_readonly("min_perturbations",
                      &PerturbationParams::minPerturbations)
        .def_readonly("max_perturbations",
                      &PerturbationParams::maxPerturbations)
        .def(py::self == py::self, py::arg("other"));  // this is __eq__

    py::class_<PerturbationManager>(
        m, "PerturbationManager", DOC(pyvrp, search, PerturbationManager))
        .def(py::init<PerturbationParams>(),
             py::arg("params") = PerturbationParams())
        .def("num_perturbations",
             &PerturbationManager::numPerturbations,
             DOC(pyvrp, search, PerturbationManager, numPerturbations))
        .def("shuffle",
             &PerturbationManager::shuffle,
             py::arg("rng"),
             DOC(pyvrp, search, PerturbationManager, shuffle))
        .def("perturb",
             &PerturbationManager::perturb,
             py::arg("solution"),
             py::arg("search_space"),
             py::arg("cost_evaluator"),
             py::call_guard<py::gil_scoped_release>(),
             DOC(pyvrp, search, PerturbationManager, perturb));

    py::class_<LocalSearch::Statistics>(
        m, "LocalSearchStatistics", DOC(pyvrp, search, LocalSearch, Statistics))
        .def_readonly("num_moves", &LocalSearch::Statistics::numMoves)
        .def_readonly("num_improving", &LocalSearch::Statistics::numImproving)
        .def_readonly("num_updates", &LocalSearch::Statistics::numUpdates);

    py::class_<LocalSearch>(m, "LocalSearch")
        .def(py::init<pyvrp::ProblemData const &,
                      std::vector<std::vector<size_t>>,
                      PerturbationManager &>(),
             py::arg("data"),
             py::arg("neighbours"),
             py::arg("perturbation_manager") = PerturbationManager(),
             py::keep_alive<1, 2>(),  // keep data alive until LS is freed
             py::keep_alive<1, 4>())  // also keep perturbation_manager alive
        .def_property("neighbours",
                      &LocalSearch::neighbours,
                      &LocalSearch::setNeighbours,
                      py::return_value_policy::reference_internal)
        .def_property_readonly("statistics", &LocalSearch::statistics)
        .def_property_readonly("unary_operators",
                               &LocalSearch::unaryOperators,
                               py::return_value_policy::reference_internal)
        .def_property_readonly("binary_operators",
                               &LocalSearch::binaryOperators,
                               py::return_value_policy::reference_internal)
        .def("add_operator",
             py::overload_cast<UnaryOperator &>(&LocalSearch::addOperator),
             py::arg("op"),
             py::keep_alive<1, 2>())
        .def("add_operator",
             py::overload_cast<BinaryOperator &>(&LocalSearch::addOperator),
             py::arg("op"),
             py::keep_alive<1, 2>())
        .def("__call__",
             &LocalSearch::operator(),
             py::arg("solution"),
             py::arg("cost_evaluator"),
             py::arg("exhaustive") = false,
             py::call_guard<py::gil_scoped_release>())
        .def("shuffle", &LocalSearch::shuffle, py::arg("rng"));

    py::class_<Solution>(m, "Solution", DOC(pyvrp, search, Solution))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_readonly("nodes", &Solution::nodes)
        .def_readonly("routes", &Solution::routes)
        .def("load", &Solution::load, py::arg("solution"))
        .def("unload", &Solution::unload)
        .def("insert",
             &Solution::insert,
             py::arg("node"),
             py::arg("search_space"),
             py::arg("cost_evaluator"),
             py::arg("required"));

    py::class_<Route>(m, "Route", DOC(pyvrp, search, Route))
        .def(py::init<pyvrp::ProblemData const &, size_t>(),
             py::arg("data"),
             py::arg("vehicle_type"),
             py::keep_alive<1, 2>())  // keep data alive
        .def_property_readonly("vehicle_type", &Route::vehicleType)
        .def("num_clients", &Route::numClients)
        .def("num_depots", &Route::numDepots)
        .def("num_trips", &Route::numTrips)
        .def("max_trips", &Route::maxTrips)
        .def(py::self == py::self, py::arg("other"))  // this is __eq__
        .def(  // __eq__ overload for pyvrp.Route
            "__eq__",
            [](Route const &route, pyvrp::Route const &other)
            { return route == other; },
            py::is_operator())
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
        .def("overtime", &Route::overtime)
        .def("prizes", &Route::prizes)
        .def("duration_cost", &Route::durationCost)
        .def("unit_duration_cost", &Route::unitDurationCost)
        .def("unit_overtime_cost", &Route::unitOvertimeCost)
        .def("has_duration_cost", &Route::hasDurationCost)
        .def("shift_duration", &Route::shiftDuration)
        .def("max_duration", &Route::maxDuration)
        .def("max_overtime", &Route::maxOvertime)
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
}
