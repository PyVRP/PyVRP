#include "Solution.h"

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>

namespace py = pybind11;

PYBIND11_MODULE(_Solution, m)
{
    py::class_<Solution::Route>(m, "Route")
        .def(py::init<ProblemData const &, std::vector<int>, size_t>(),
             py::arg("data"),
             py::arg("visits"),
             py::arg("vehicle_type"))
        .def("visits",
             &Solution::Route::visits,
             py::return_value_policy::reference_internal)
        .def(
            "distance",
            [](Solution::Route const &route) { return route.distance().get(); })
        .def("demand",
             [](Solution::Route const &route) { return route.demand().get(); })
        .def("excess_load",
             [](Solution::Route const &route) {
                 return route.excessLoad().get();
             })
        .def(
            "duration",
            [](Solution::Route const &route) { return route.duration().get(); })
        .def("service_duration",
             [](Solution::Route const &route) {
                 return route.serviceDuration().get();
             })
        .def(
            "time_warp",
            [](Solution::Route const &route) { return route.timeWarp().get(); })
        .def("wait_duration",
             [](Solution::Route const &route) {
                 return route.waitDuration().get();
             })
        .def("release_time",
             [](Solution::Route const &route) {
                 return route.releaseTime().get();
             })
        .def("prizes",
             [](Solution::Route const &route) { return route.prizes().get(); })
        .def("centroid", &Solution::Route::centroid)
        .def("vehicle_type", &Solution::Route::vehicleType)
        .def("is_feasible", &Solution::Route::isFeasible)
        .def("has_excess_load", &Solution::Route::hasExcessLoad)
        .def("has_time_warp", &Solution::Route::hasTimeWarp)
        .def("__len__", &Solution::Route::size)
        .def(
            "__iter__",
            [](Solution::Route const &route) {
                return py::make_iterator(route.cbegin(), route.cend());
            },
            py::return_value_policy::reference_internal)
        .def(
            "__getitem__",
            [](Solution::Route const &route, int idx) {
                // int so we also support negative offsets from the end.
                idx = idx < 0 ? route.size() + idx : idx;
                if (idx < 0 || static_cast<size_t>(idx) >= route.size())
                    throw py::index_error();
                return route[idx];
            },
            py::arg("idx"))
        .def(py::self == py::self)  // this is __eq__
        .def("__str__", [](Solution::Route const &route) {
            std::stringstream stream;
            stream << route;
            return stream.str();
        });

    py::class_<Solution>(m, "Solution")
        // Note, the order of constructors is important! Since Solution::Route
        // implements __len__ and __getitem__, it can also be converted to
        // std::vector<int> and thus a list of Routes is a valid argument for
        // both constructors. We want to avoid using the second constructor
        // since that would loose the vehicle types associations. As pybind
        // will use the first matching constructor we put this one first.
        .def(py::init<ProblemData const &,
                      std::vector<Solution::Route> const &>(),
             py::arg("data"),
             py::arg("routes"))
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
                        return Solution(data, rng);
                    },
                    py::arg("data"),
                    py::arg("rng"));
            })
        .def("num_routes", &Solution::numRoutes)
        .def("num_clients", &Solution::numClients)
        .def("get_routes",
             &Solution::getRoutes,
             py::return_value_policy::reference_internal)
        .def("get_neighbours",
             &Solution::getNeighbours,
             py::return_value_policy::reference_internal)
        .def("is_feasible", &Solution::isFeasible)
        .def("has_excess_load", &Solution::hasExcessLoad)
        .def("has_time_warp", &Solution::hasTimeWarp)
        .def("distance",
             [](Solution const &sol) { return sol.distance().get(); })
        .def("excess_load",
             [](Solution const &sol) { return sol.excessLoad().get(); })
        .def("time_warp",
             [](Solution const &sol) { return sol.timeWarp().get(); })
        .def("prizes", [](Solution const &sol) { return sol.prizes().get(); })
        .def("uncollected_prizes",
             [](Solution const &sol) { return sol.uncollectedPrizes().get(); })
        .def("__copy__", [](Solution const &sol) { return Solution(sol); })
        .def(
            "__deepcopy__",
            [](Solution const &sol, py::dict) { return Solution(sol); },
            py::arg("memo"))
        .def("__hash__",
             [](Solution const &sol) { return std::hash<Solution>()(sol); })
        .def(py::self == py::self)  // this is __eq__
        .def("__str__", [](Solution const &sol) {
            std::stringstream stream;
            stream << sol;
            return stream.str();
        });
}
