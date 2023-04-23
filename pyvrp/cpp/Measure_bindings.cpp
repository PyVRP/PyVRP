#include "Measure.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(_Measure, m)
{
    py::class_<Measure<MeasureType::DISTANCE>>(m, "distance_type");
    py::class_<Measure<MeasureType::DURATION>>(m, "duration_type");
    py::class_<Measure<MeasureType::COST>>(m, "cost_type");
}
