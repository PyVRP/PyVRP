#include "Population.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_Population(py::module_ &m)
{
    py::class_<Population>(m, "Population")
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      XorShift128 &,
                      DiversityMeasure,
                      PopulationParams>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"),
             py::arg("op"),
             py::arg("params"))
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      XorShift128 &,
                      DiversityMeasure>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"),
             py::arg("op"))
        .def("add", &Population::add, py::arg("individual"));
}
