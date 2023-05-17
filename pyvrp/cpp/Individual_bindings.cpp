#include "Individual.h"

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>

namespace py = pybind11;

PYBIND11_MODULE(_Individual, m)
{
    py::class_<Individual::Route>(m, "Route")
        .def(py::init<ProblemData const &, std::vector<int>>(),
             py::arg("data"),
             py::arg("visits"))
        .def("visits",
             &Individual::Route::visits,
             py::return_value_policy::reference_internal)
        .def("distance", &Individual::Route::distance)
        .def("demand", &Individual::Route::demand)
        .def("excess_load", &Individual::Route::excessLoad)
        .def("duration", &Individual::Route::duration)
        .def("service_duration", &Individual::Route::serviceDuration)
        .def("time_warp", &Individual::Route::timeWarp)
        .def("wait_duration", &Individual::Route::waitDuration)
        .def("prizes", &Individual::Route::prizes)
        .def("is_feasible", &Individual::Route::isFeasible)
        .def("has_excess_load", &Individual::Route::hasExcessLoad)
        .def("has_time_warp", &Individual::Route::hasTimeWarp)
        .def("__len__", &Individual::Route::size)
        .def(
            "__iter__",
            [](Individual::Route const &route) {
                return py::make_iterator(route.cbegin(), route.cend());
            },
            py::return_value_policy::reference_internal)
        .def(
            "__getitem__",
            [](Individual::Route const &route, int idx) {
                // int so we also support negative offsets from the end.
                idx = idx < 0 ? route.size() + idx : idx;
                if (idx < 0 || static_cast<size_t>(idx) >= route.size())
                    throw py::index_error();
                return route[idx];
            },
            py::arg("idx"))
        .def("__str__", [](Individual::Route const &route) {
            std::stringstream stream;
            stream << route;
            return stream.str();
        });

    py::class_<Individual>(m, "Individual")
        .def(py::init<ProblemData const &,
                      std::vector<std::vector<int>> const &>(),
             py::arg("data"),
             py::arg("routes"))
        .def_property_readonly_static(
            "make_random",                // this is a bit of a workaround for
            [](py::object)                // classmethods, because pybind does
            {                             // not yet support those natively.
                return py::cpp_function(  // See issue 1693 in the pybind repo.
                    [](ProblemData const &data, XorShift128 &rng) {
                        return Individual(data, rng);
                    },
                    py::arg("data"),
                    py::arg("rng"));
            })
        .def("num_routes", &Individual::numRoutes)
        .def("num_clients", &Individual::numClients)
        .def("get_routes",
             &Individual::getRoutes,
             py::return_value_policy::reference_internal)
        .def("get_neighbours",
             &Individual::getNeighbours,
             py::return_value_policy::reference_internal)
        .def("is_feasible", &Individual::isFeasible)
        .def("has_excess_load", &Individual::hasExcessLoad)
        .def("has_time_warp", &Individual::hasTimeWarp)
        .def("distance", &Individual::distance)
        .def("excess_load", &Individual::excessLoad)
        .def("time_warp", &Individual::timeWarp)
        .def("prizes", &Individual::prizes)
        .def("uncollected_prizes", &Individual::uncollectedPrizes)
        .def(
            "__copy__",
            [](Individual const &individual) { return Individual(individual); })
        .def(
            "__deepcopy__",
            [](Individual const &individual, py::dict) {
                return Individual(individual);
            },
            py::arg("memo"))
        .def("__hash__",
             [](Individual const &individual) {
                 return std::hash<Individual>()(individual);
             })
        .def(pybind11::self == pybind11::self)  // this is __eq__
        .def("__str__", [](Individual const &individual) {
            std::stringstream stream;
            stream << individual;
            return stream.str();
        });
}
