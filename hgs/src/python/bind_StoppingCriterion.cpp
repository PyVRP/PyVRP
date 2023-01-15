#include "StoppingCriterion.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_StoppingCriterion(py::module_ &m)
{
    py::class_<StoppingCriterion>(m, "StoppingCriterion");
}
