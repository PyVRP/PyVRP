#include "Individual.h"

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>

namespace py = pybind11;

PYBIND11_MODULE(_Individual, m)
{
    py::class_<Individual>(m, "Individual")
        .def(py::init<ProblemData const &,
                      PenaltyManager const &,
                      std::vector<std::vector<int>>>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("routes"),
             py::keep_alive<1, 3>())  // keep penalty_manager alive
        .def_property_readonly_static(
            "make_random",                // this is a bit of a workaround for
            [](py::object)                // classmethods, because pybind does
            {                             // not yet support those natively.
                return py::cpp_function(  // See issue 1693 in the pybind repo.
                    [](ProblemData const &data,
                       PenaltyManager const &penaltyManager,
                       XorShift128 &rng)
                    { return Individual(data, penaltyManager, rng); },
                    py::arg("data"),
                    py::arg("penalty_manager"),
                    py::arg("routes"),
                    py::keep_alive<0, 2>());  // keep penalty_manager alive
            })
        .def("cost", &Individual::cost)
        .def("num_routes", &Individual::numRoutes)
        .def("get_routes",
             &Individual::getRoutes,
             py::return_value_policy::reference_internal)
        .def("get_neighbours",
             &Individual::getNeighbours,
             py::return_value_policy::reference_internal)
        .def("is_feasible", &Individual::isFeasible)
        .def("has_excess_capacity", &Individual::hasExcessCapacity)
        .def("has_time_warp", &Individual::hasTimeWarp)
        .def("__copy__",
             [](Individual const &individual)
             { return Individual(individual); })
        .def(
            "__deepcopy__",
            [](Individual const &individual, py::dict)
            { return Individual(individual); },
            py::arg("memo"))
        .def("__hash__",
             [](Individual const &individual)
             { return std::hash<Individual>()(individual); })
        .def(pybind11::self == pybind11::self)  // this is __eq__
        .def("__str__",
             [](Individual const &individual)
             {
                 std::stringstream stream;
                 stream << individual;
                 return stream.str();
             });
}
