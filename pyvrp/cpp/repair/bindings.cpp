#include "greedy_repair.h"
#include "repair_docs.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_repair, m)
{
    m.def("greedy_repair",
          &pyvrp::repair::greedyRepair,
          py::arg("solution"),
          py::arg("unplanned"),
          py::arg("data"),
          py::arg("cost_evaluator"),
          DOC(pyvrp, repair, greedyRepair));
}
