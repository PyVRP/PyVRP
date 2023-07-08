#include "crossover.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

using namespace pyvrp::crossover;

PYBIND11_MODULE(_crossover, m)
{
    m.def("selective_route_exchange",
          &selectiveRouteExchange,
          py::arg("parents"),
          py::arg("data"),
          py::arg("cost_evaluator"),
          py::arg("start_indices"),
          py::arg("num_moved_routes"));
}
