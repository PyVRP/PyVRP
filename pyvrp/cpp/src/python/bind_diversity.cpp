#include "bindings.h"
#include "diversity.h"

namespace py = pybind11;

void bind_diversity(py::module_ &m)
{
    m.def("broken_pairs_distance", &brokenPairsDistance);
}
