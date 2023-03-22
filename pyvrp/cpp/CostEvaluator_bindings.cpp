#include "CostEvaluator.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_CostEvaluator, m)
{
    py::class_<CostEvaluator>(m, "CostEvaluator")
        .def(py::init<unsigned int, unsigned int>(),
             py::arg("capacity_penalty"),
             py::arg("tw_penalty"))
        .def_property_readonly_static(
            "get_default",                // this is a bit of a workaround for
            [](py::object)                // classmethods, because pybind does
            {                             // not yet support those natively.
                return py::cpp_function(  // See issue 1693 in the pybind repo.
                    []() { return CostEvaluator(0, 0); });
            })
        .def("load_penalty",
             &CostEvaluator::loadPenalty,
             py::arg("load"),
             py::arg("vehicle_capacity"))
        .def("tw_penalty", &CostEvaluator::twPenalty, py::arg("time_warp"))
        .def("penalized_cost",
             &CostEvaluator::penalizedCost,
             py::arg("individual"))
        .def("cost", &CostEvaluator::penalizedCost, py::arg("individual"));
}
