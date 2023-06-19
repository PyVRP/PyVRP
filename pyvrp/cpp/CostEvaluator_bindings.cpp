#include "CostEvaluator.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_CostEvaluator, m)
{
    py::class_<CostEvaluator>(m, "CostEvaluator")
        .def(py::init([](unsigned int capacityPenalty, unsigned int twPenalty) {
                 return CostEvaluator(capacityPenalty, twPenalty);
             }),
             py::arg("capacity_penalty") = 0,
             py::arg("tw_penalty") = 0)
        .def(
            "load_penalty",
            [](CostEvaluator const &evaluator,
               Value load,
               Value vehicleCapacity) {
                return evaluator.loadPenalty(load, vehicleCapacity).get();
            },
            py::arg("load"),
            py::arg("vehicle_capacity"))
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
