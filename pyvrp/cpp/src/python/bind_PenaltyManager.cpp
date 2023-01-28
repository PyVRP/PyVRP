#include "PenaltyManager.h"
#include "bindings.h"

namespace py = pybind11;

void bind_PenaltyManager(py::module_ &m)
{
    py::class_<PenaltyManager>(m, "PenaltyManager")
        .def(py::init<unsigned int, PenaltyParams>(),
             py::arg("vehicle_capacity"),
             py::arg("params"))
        .def(py::init<unsigned int>(), py::arg("vehicle_capacity"));
}
