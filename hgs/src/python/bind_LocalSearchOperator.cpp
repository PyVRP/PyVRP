#include "LocalSearchOperator.h"
#include "bindings.h"

namespace py = pybind11;

void bind_LocalSearchOperator(py::module_ &m)
{
    py::class_<LocalSearchOperator<Node>>(m, "NodeLocalSearchOperator");
    py::class_<LocalSearchOperator<Route>>(m, "RouteLocalSearchOperator");
}
