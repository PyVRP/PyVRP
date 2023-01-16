#include "crossover.h"
#include "bindings.h"

namespace py = pybind11;

void bind_crossover(py::module_ &m)
{
    m.def("selective_route_exchange", &selectiveRouteExchange);
}
