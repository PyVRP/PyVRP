#include "crossover_docs.h"
#include "ordered_crossover.h"
#include "selective_route_exchange.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_crossover, m)
{
    m.def("ordered_crossover",
          &pyvrp::crossover::orderedCrossover,
          py::arg("parents"),
          py::arg("data"),
          py::arg("indices"),
          py::call_guard<py::gil_scoped_release>(),
          DOC(pyvrp, crossover, orderedCrossover));

    m.def("selective_route_exchange",
          &pyvrp::crossover::selectiveRouteExchange,
          py::arg("parents"),
          py::arg("data"),
          py::arg("cost_evaluator"),
          py::arg("start_indices"),
          py::arg("num_moved_routes"),
          py::call_guard<py::gil_scoped_release>(),
          DOC(pyvrp, crossover, selectiveRouteExchange));
}
