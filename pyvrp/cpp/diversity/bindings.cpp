#include "diversity.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

using namespace pyvrp::diversity;

PYBIND11_MODULE(_diversity, m)
{
    m.def("broken_pairs_distance",
          &brokenPairsDistance,
          py::arg("first"),
          py::arg("second"));
}
