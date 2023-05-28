#include "CostEvaluator.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_CostEvaluator, m)
{
    py::class_<CostEvaluator>(m, "CostEvaluator")
        .def(py::init<unsigned int, unsigned int>(),
             py::arg("capacity_penalty") = 0,
             py::arg("tw_penalty") = 0)
        .def(
            "load_penalty",
            [](CostEvaluator const &evaluator,
               unsigned int load,
               unsigned int vehicleCapacity) {
                auto penalty = evaluator.loadPenalty(load, vehicleCapacity);
                return static_cast<value_type>(penalty);
            },
            py::arg("load"),
            py::arg("vehicle_capacity"))
        .def(
            "tw_penalty",
            [](CostEvaluator const &evaluator, value_type const timeWarp) {
                auto penalty = evaluator.twPenalty(timeWarp);
                return static_cast<value_type>(penalty);
            },
            py::arg("time_warp"))
        .def(
            "penalised_cost",
            [](CostEvaluator const &evaluator, Individual const &individual) {
                auto cost = evaluator.penalisedCost(individual);
                return static_cast<value_type>(cost);
            },
            py::arg("individual"))
        .def(
            "cost",
            [](CostEvaluator const &evaluator, Individual const &individual) {
                auto cost = evaluator.cost(individual);
                return static_cast<value_type>(cost);
            },
            py::arg("individual"));
}
