#include "CostEvaluator.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_CostEvaluator, m)
{
    py::class_<CostEvaluator>(m, "CostEvaluator")
        .def(py::init([](unsigned int weightCapacityPenalty, unsigned int volumeCapacityPenalty, unsigned int twPenalty) {
                 return CostEvaluator(weightCapacityPenalty, volumeCapacityPenalty, twPenalty);
             }),
             py::arg("weight_capacity_penalty") = 0,
             py::arg("volume_capacity_penalty") = 0,
             py::arg("tw_penalty") = 0)
        .def(
            "load_weight_penalty",
            [](CostEvaluator const &evaluator,
               Value load_weight,
               Value weight_capacity) {
                return evaluator.weightPenalty(load_weight, weight_capacity).get();
            },
            py::arg("load_weight"),
            py::arg("weight_capacity"))
        .def(
            "load_volume_penalty",
            [](CostEvaluator const &evaluator,
               Value load_volume,
               Value volume_capacity) {
                return evaluator.volumePenalty(load_volume, volume_capacity).get();
            },
            py::arg("load_volume"),
            py::arg("volume_capacity"))
        .def(
            "tw_penalty",
            [](CostEvaluator const &evaluator, Value const timeWarp) {
                return evaluator.twPenalty(timeWarp).get();
            },
            py::arg("time_warp"))
        .def(
            "penalised_cost",
            [](CostEvaluator const &evaluator, Solution const &solution) {
                return evaluator.penalisedCost(solution).get();
            },
            py::arg("solution"))
        .def(
            "cost",
            [](CostEvaluator const &evaluator, Solution const &solution) {
                return evaluator.cost(solution).get();
            },
            py::arg("solution"));
}
