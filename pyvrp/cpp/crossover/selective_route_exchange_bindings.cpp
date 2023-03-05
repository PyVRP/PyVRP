#include "crossover.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_selective_route_exchange, m)
{
    m.def("selective_route_exchange",
          &selectiveRouteExchange,
          py::arg("parents"),
          py::arg("data"),
          py::arg("penalty_manager"),
          py::arg("start_a"),
          py::arg("start_b"),
          py::arg("num_moved_routes"));
}
