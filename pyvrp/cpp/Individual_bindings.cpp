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
             py::arg("rng"))
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      std::vector<std::vector<int>>>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("routes"))
        .def(py::init<Individual const &>(), py::arg("individual"))
        .def("cost", &Individual::cost)
        .def("num_routes", &Individual::numRoutes)
        .def("get_routes", &Individual::getRoutes)
        .def("get_neighbours", &Individual::getNeighbours)
        .def("is_feasible", &Individual::isFeasible)
        .def("has_excess_capacity", &Individual::hasExcessCapacity)
        .def("has_time_warp", &Individual::hasTimeWarp)
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
