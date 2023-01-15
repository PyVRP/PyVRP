#include "GeneticAlgorithmParams.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_GeneticAlgorithmParams(py::module_ &m)
{
    py::class_<GeneticAlgorithmParams>(m, "GeneticAlgorithmParams")
        .def(py::init<size_t, size_t, bool, bool>(),
             py::arg("nb_penalty_management") = 47,
             py::arg("repair_probability") = 79,
             py::arg("collect_statistics") = false,
             py::arg("should_intensify") = true)
        .def_readonly("nb_penalty_management",
                      &GeneticAlgorithmParams::nbPenaltyManagement)
        .def_readonly("repair_probability",
                      &GeneticAlgorithmParams::repairProbability)
        .def_readonly("collect_statistics",
                      &GeneticAlgorithmParams::collectStatistics)
        .def_readonly("should_intensify",
                      &GeneticAlgorithmParams::shouldIntensify);
}
