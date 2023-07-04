#include "ProblemData.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(_ProblemData, m)
{
    py::class_<ProblemData::Client>(m, "Client")
        .def(py::init<Value,
                      Value,
                      Value,
                      Value,
                      Value,
                      Value,
                      Value,
                      Value,
                      bool>(),
             py::arg("x"),
             py::arg("y"),
             py::arg("demand") = 0,
             py::arg("service_duration") = 0,
             py::arg("tw_early") = 0,
             py::arg("tw_late") = 0,
             py::arg("release_time") = 0,
             py::arg("prize") = 0,
             py::arg("required") = true)
        .def_property_readonly(
            "x",
            [](ProblemData::Client const &client) { return client.x.get(); })
        .def_property_readonly(
            "y",
            [](ProblemData::Client const &client) { return client.y.get(); })
        .def_property_readonly("demand",
                               [](ProblemData::Client const &client) {
                                   return client.demand.get();
                               })
        .def_property_readonly("service_duration",
                               [](ProblemData::Client const &client) {
                                   return client.serviceDuration.get();
                               })
        .def_property_readonly("tw_early",
                               [](ProblemData::Client const &client) {
                                   return client.twEarly.get();
                               })
        .def_property_readonly("tw_late",
                               [](ProblemData::Client const &client) {
                                   return client.twLate.get();
                               })
        .def_property_readonly("release_time",
                               [](ProblemData::Client const &client) {
                                   return client.releaseTime.get();
                               })
        .def_property_readonly("prize",
                               [](ProblemData::Client const &client) {
                                   return client.prize.get();
                               })
        .def_readonly("required", &ProblemData::Client::required);

    py::class_<ProblemData::VehicleType>(m, "VehicleType")
        .def(py::init<Value, size_t>(),
             py::arg("capacity"),
             py::arg("num_available"))
        .def_property_readonly("capacity",
                               [](ProblemData::VehicleType const &vehicleType) {
                                   return vehicleType.capacity.get();
                               })
        .def_readonly("num_available", &ProblemData::VehicleType::numAvailable);

    py::class_<ProblemData>(m, "ProblemData")
        .def(py::init(
                 [](std::vector<ProblemData::Client> const &clients,
                    std::vector<ProblemData::VehicleType> const &vehicleTypes,
                    std::vector<std::vector<Value>> const &dist,
                    std::vector<std::vector<Value>> const &dur) {
                     Matrix<Distance> distMat(clients.size());
                     Matrix<Duration> durMat(clients.size());

                     for (size_t row = 0; row != clients.size(); ++row)
                         for (size_t col = 0; col != clients.size(); ++col)
                         {
                             distMat(row, col) = dist[row][col];
                             durMat(row, col) = dur[row][col];
                         }

                     return ProblemData(clients, vehicleTypes, distMat, durMat);
                 }),
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
             py::return_value_policy::reference_internal)
        .def("depot",
             &ProblemData::depot,
             py::return_value_policy::reference_internal)
        .def("centroid",
             &ProblemData::centroid,
             py::return_value_policy::reference_internal)
        .def("vehicle_type",
             &ProblemData::vehicleType,
             py::arg("vehicle_type"),
             py::return_value_policy::reference_internal)
        .def(
            "dist",
            [](ProblemData const &data, size_t first, size_t second) {
                return data.dist(first, second).get();
            },
            py::arg("first"),
            py::arg("second"))
        .def(
            "duration",
            [](ProblemData const &data, size_t first, size_t second) {
                return data.duration(first, second).get();
            },
            py::arg("first"),
            py::arg("second"));
}
