#include "SubPopulation.h"

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(SubPopulation, m)
{
    py::class_<PopulationParams>(m, "PopulationParams")
        .def(py::init<size_t, size_t, size_t, size_t, double, double>(),
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

    py::class_<SubPopulation::Item>(m, "SubPopulationItem")
        .def_readonly("individual",
                      &SubPopulation::Item::individual,
                      py::return_value_policy::reference_internal)
        .def_readonly("fitness", &SubPopulation::Item::fitness)
        .def_readonly("_proximity",
                      &SubPopulation::Item::proximity,
                      py::return_value_policy::reference_internal);

    py::class_<SubPopulation>(m, "SubPopulation")
        .def(py::init<DiversityMeasure,
                      PopulationParams const &>(),
             py::arg("diversity_op"),
             py::arg("params"))
        .def("add", &SubPopulation::add, py::arg("individual"))
        .def("__len__", &SubPopulation::size)
        .def(
            "__getitem__",
            [](SubPopulation const &subPop, int idx)
            {
                // int so we also support negative offsets from the end.
                idx = idx < 0 ? subPop.size() + idx : idx;
                if (idx < 0 || static_cast<size_t>(idx) >= subPop.size())
                    throw py::index_error();
                return subPop[idx].individual;
            },
            py::arg("idx"),
            py::return_value_policy::reference_internal)
        // TODO how do we make iterator return individuals?
        // .def(
        //     "__iter__",
        //     [](SubPopulation const &subPop)
        //     { return py::make_iterator(subPop.cbegin(), subPop.cend()); },
        //     py::return_value_policy::reference_internal)
        .def("purge", &SubPopulation::purge)
        .def("update_fitness", &SubPopulation::updateFitness)
        .def("avg_distance_closest",
             &SubPopulation::avgDistanceClosest,
             py::arg("idx"))
        .def("get_biased_fitness",
             [](SubPopulation const &subPop, int idx)
             {
                 return subPop[idx].fitness;
             },
             py::arg("idx"))
        .def("_get_proximity",
             [](SubPopulation const &subPop, int idx)
             {
                 return subPop[idx].proximity;
             },
             py::arg("idx"));
}
