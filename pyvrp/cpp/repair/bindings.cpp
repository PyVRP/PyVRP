#include "greedy_repair.h"
#include "nearest_route_insert.h"
#include "repair_docs.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(_repair, m)
{
    m.def("greedy_repair",
          &pyvrp::repair::greedyRepair,
          py::arg("routes"),
          py::arg("unplanned"),
          py::arg("data"),
          py::arg("cost_evaluator"),
          DOC(pyvrp, repair, greedyRepair));

    m.def("nearest_route_insert",
          &pyvrp::repair::nearestRouteInsert,
          py::arg("routes"),
          py::arg("unplanned"),
          py::arg("data"),
          py::arg("cost_evaluator"),
          DOC(pyvrp, repair, nearestRouteInsert));
}
