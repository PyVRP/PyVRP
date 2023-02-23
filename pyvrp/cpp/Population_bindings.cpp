#include "Population.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(Population, m)
{
    py::class_<PopulationParams>(m, "PopulationParams")
        .def(py::init<size_t,
                      size_t,
                      size_t,
                      size_t,
                      double,
                      double>(),
             py::arg("min_pop_size") = 25,
             py::arg("generation_size") = 40,
             py::arg("nb_elite") = 4,
             py::arg("nb_close") = 5,
             py::arg("lb_diversity") = 0.1,
             py::arg("ub_diversity") = 0.5)
        .def_readwrite("min_pop_size", &PopulationParams::minPopSize)
        .def_readwrite("generation_size", &PopulationParams::generationSize)
        .def_property_readonly("max_pop_size", &PopulationParams::maxPopSize)
        .def_readwrite("nb_elite", &PopulationParams::nbElite)
        .def_readwrite("nb_close", &PopulationParams::nbClose)
        .def_readwrite("lb_diversity", &PopulationParams::lbDiversity)
        .def_readwrite("ub_diversity", &PopulationParams::ubDiversity);


}
