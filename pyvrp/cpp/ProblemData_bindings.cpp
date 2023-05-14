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

    py::class_<ProblemData>(m, "ProblemData")
        .def(py::init<std::vector<ProblemData::Client> const &,
                      int,
                      int,
                      std::vector<std::vector<int>> const &,
                      std::vector<std::vector<int>> const &>(),
             py::arg("clients"),
             py::arg("nb_vehicles"),
             py::arg("vehicle_cap"),
             py::arg("distance_matrix"),
             py::arg("duration_matrix"))
        .def_property_readonly("num_clients", &ProblemData::numClients)
        .def_property_readonly("num_vehicles", &ProblemData::numVehicles)
        .def_property_readonly("vehicle_capacity",
                               &ProblemData::vehicleCapacity)
        .def("client",
             &ProblemData::client,
             py::arg("client"),
             py::return_value_policy::reference)
        .def("depot", &ProblemData::depot, py::return_value_policy::reference)
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
