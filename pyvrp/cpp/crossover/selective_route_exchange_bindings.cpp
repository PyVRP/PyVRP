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
          py::arg("idx1"),
          py::arg("idx2"),
          py::arg("n_routes_to_move"));
}
