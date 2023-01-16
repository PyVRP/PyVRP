#include "StoppingCriterion.h"
#include "bindings.h"

namespace py = pybind11;

void bind_StoppingCriterion(py::module_ &m)
{
    py::class_<StoppingCriterion>(m, "StoppingCriterion");
}
