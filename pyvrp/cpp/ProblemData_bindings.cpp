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
        .def_property_readonly("prize",
                               [](ProblemData::Client const &client) {
                                   return client.prize.get();
                               })
        .def_readonly("required", &ProblemData::Client::required);

    py::class_<ProblemData>(m, "ProblemData")
        .def(py::init([](std::vector<ProblemData::Client> const &clients,
                         int numVehicles,
                         int vehicleCap,
                         std::vector<std::vector<int>> const &dist,
                         std::vector<std::vector<int>> const &dur) {
                 Matrix<Distance> distMat(clients.size());
                 Matrix<Duration> durMat(clients.size());

                 for (size_t row = 0; row != clients.size(); ++row)
                     for (size_t col = 0; col != clients.size(); ++col)
                     {
                         distMat(row, col) = dist[row][col];
                         durMat(row, col) = dur[row][col];
                     }

                 return ProblemData(
                     clients, numVehicles, vehicleCap, distMat, durMat);
             }),
             py::arg("clients"),
             py::arg("num_vehicles"),
             py::arg("vehicle_cap"),
             py::arg("distance_matrix"),
             py::arg("duration_matrix"))
        .def_property_readonly("num_clients", &ProblemData::numClients)
        .def_property_readonly("num_vehicles", &ProblemData::numVehicles)
        .def_property_readonly("vehicle_capacity",
                               [](ProblemData const &data) {
                                   return data.vehicleCapacity().get();
                               })
        .def("client",
             &ProblemData::client,
             py::arg("client"),
             py::return_value_policy::reference_internal)
        .def("depot", &ProblemData::depot, py::return_value_policy::reference)
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
