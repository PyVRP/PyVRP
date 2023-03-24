#include "CostEvaluator.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_CostEvaluator, m)
{
    py::class_<CostEvaluator>(m, "CostEvaluator")
        .def(py::init<unsigned int, unsigned int>(),
             py::arg("capacity_penalty") = 0,
             py::arg("tw_penalty") = 0)
        .def("load_penalty",
             &CostEvaluator::loadPenalty,
             py::arg("load"),
             py::arg("vehicle_capacity"))
        .def("tw_penalty", &CostEvaluator::twPenalty, py::arg("time_warp"))
        .def("penalised_cost",
             &CostEvaluator::penalisedCost,
             py::arg("individual"))
        .def("cost", &CostEvaluator::cost, py::arg("individual"));
}
