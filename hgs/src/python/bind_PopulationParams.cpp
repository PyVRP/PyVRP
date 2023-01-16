#include "PopulationParams.h"
#include "bindings.h"

namespace py = pybind11;

void bind_PopulationParams(py::module_ &m)
{
    py::class_<PopulationParams>(m, "PopulationParams")
        .def(py::init<size_t, size_t, size_t, size_t, double, double>(),
             py::arg("min_pop_size") = 25,
             py::arg("generation_size") = 40,
             py::arg("nb_elite") = 4,
             py::arg("nb_close") = 5,
             py::arg("lb_diversity") = 0.1,
             py::arg("ub_diversity") = 0.5)
        .def_readonly("min_pop_size", &PopulationParams::minPopSize)
        .def_readonly("generation_size", &PopulationParams::generationSize)
        .def_readonly("nb_elite", &PopulationParams::nbElite)
        .def_readonly("nb_close", &PopulationParams::nbClose)
        .def_readonly("lb_diversity", &PopulationParams::lbDiversity)
        .def_readonly("ub_diversity", &PopulationParams::ubDiversity);
}
