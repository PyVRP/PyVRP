#include "diversity.h"
#include "diversity_docs.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(_diversity, m)
{
    m.def("broken_pairs_distance",
          &pyvrp::diversity::brokenPairsDistance,
          py::arg("first"),
          py::arg("second"),
          DOC(pyvrp, diversity, brokenPairsDistance));
    m.def("different_neighbours",
          &pyvrp::diversity::differentNeighbours,
          py::arg("first"),
          py::arg("second"));
}
