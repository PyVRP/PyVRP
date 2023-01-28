#include "GeneticAlgorithm.h"
#include "bindings.h"

namespace py = pybind11;

void bind_GeneticAlgorithm(py::module_ &m)
{
    py::class_<GeneticAlgorithm>(m, "GeneticAlgorithm")
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      XorShift128 &,
                      Population &,
                      LocalSearch &,
                      CrossoverOperator,
                      GeneticAlgorithmParams>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"),
             py::arg("population"),
             py::arg("local_search"),
             py::arg("crossover_operator"),
             py::arg("params"))
        .def(py::init<ProblemData &,
                      PenaltyManager &,
                      XorShift128 &,
                      Population &,
                      LocalSearch &,
                      CrossoverOperator>(),
             py::arg("data"),
             py::arg("penalty_manager"),
             py::arg("rng"),
             py::arg("population"),
             py::arg("local_search"),
             py::arg("crossover_operator"))
        .def("run", &GeneticAlgorithm::run, py::arg("stop"));
}
