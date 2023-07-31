#include "repair.h"
#include "repair_docs.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_repair, m)
{
    m.def("greedy_repair",
          &pyvrp::repair::greedyRepair,
          DOC(pyvrp, repair, greedyRepair));
}
