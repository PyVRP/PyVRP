#include "Individual.h"

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>

namespace py = pybind11;

PYBIND11_MODULE(_Individual, m)
{
    py::class_<Individual>(m, "Individual")
        .def(py::init<ProblemData &, PenaltyManager &, XorShift128 &>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"),
             py::keep_alive<1, 2>(),  // keep data and penalty_manager alive
             py::keep_alive<1, 3>())  // at least until individual is freed
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      std::vector<std::vector<int>>>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("routes"),
             py::keep_alive<1, 2>(),  // keep data and penalty_manager alive
             py::keep_alive<1, 3>())  // at least until individual is freed
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
        .def("__eq__", &Individual::operator==)
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
