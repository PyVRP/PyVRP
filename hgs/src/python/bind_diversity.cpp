#include "diversity.h"
#include "bindings.h"

namespace py = pybind11;

void bind_diversity(py::module_ &m)
{
    m.def("broken_pairs_distance", &brokenPairsDistance);
}
