#include "ProblemData.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(_ProblemData, m)
{
    py::class_<ProblemData::Client>(m, "Client")
        .def(py::init<int, int, int, int, int, int, int, bool>(),
             py::arg("x"),
             py::arg("y"),
             py::arg("demand") = 0,
             py::arg("service_duration") = 0,
             py::arg("tw_early") = 0,
             py::arg("tw_late") = 0,
             py::arg("prize") = 0,
             py::arg("required") = true)
        .def_readonly("x", &ProblemData::Client::x)
        .def_readonly("y", &ProblemData::Client::y)
        .def_readonly("service_duration", &ProblemData::Client::serviceDuration)
        .def_readonly("demand", &ProblemData::Client::demand)
        .def_readonly("tw_early", &ProblemData::Client::twEarly)
        .def_readonly("tw_late", &ProblemData::Client::twLate)
        .def_readonly("prize", &ProblemData::Client::prize)
        .def_readonly("required", &ProblemData::Client::required);

    py::class_<ProblemData::VehicleType>(m, "VehicleType")
        .def(py::init<int, size_t>(),
             py::arg("capacity"),
             py::arg("qty_available"))
        .def_readonly("capacity", &ProblemData::VehicleType::capacity)
        .def_readonly("qty_available",
                      &ProblemData::VehicleType::qty_available);

    py::class_<ProblemData>(m, "ProblemData")
        .def(py::init<std::vector<ProblemData::Client> const &,
                      std::vector<ProblemData::VehicleType> const &,
                      std::vector<std::vector<int>> const &,
                      std::vector<std::vector<int>> const &>(),
             py::arg("clients"),
             py::arg("vehicle_types"),
             py::arg("distance_matrix"),
             py::arg("duration_matrix"))
        .def_property_readonly("num_clients", &ProblemData::numClients)
        .def_property_readonly("num_vehicles", &ProblemData::numVehicles)
        .def_property_readonly("num_vehicle_types",
                               &ProblemData::numVehicleTypes)
        .def("client",
             &ProblemData::client,
             py::arg("client"),
             py::return_value_policy::reference)
        .def("depot", &ProblemData::depot, py::return_value_policy::reference)
        .def("vehicle_type",
             &ProblemData::vehicleType,
             py::arg("idx"),
             py::return_value_policy::reference)
        .def("dist", &ProblemData::dist, py::arg("first"), py::arg("second"))
        .def("duration",
             &ProblemData::duration,
             py::arg("first"),
             py::arg("second"))
        .def("distance_matrix",
             &ProblemData::distanceMatrix,
             py::return_value_policy::reference)
        .def("duration_matrix",
             &ProblemData::durationMatrix,
             py::return_value_policy::reference);
}
