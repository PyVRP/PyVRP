#include "DynamicBitset.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_DynamicBitset, m)
{
    py::class_<DynamicBitset>(m, "DynamicBitset");
}
