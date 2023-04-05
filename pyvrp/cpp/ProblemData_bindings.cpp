#include "ProblemData.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(_ProblemData, m)
{
    py::class_<ProblemData::Client>(m, "Client")
        .def_readonly("x", &ProblemData::Client::x)
        .def_readonly("y", &ProblemData::Client::y)
        .def_readonly("service_duration", &ProblemData::Client::serviceDuration)
        .def_readonly("demand", &ProblemData::Client::demand)
        .def_readonly("tw_early", &ProblemData::Client::twEarly)
        .def_readonly("tw_late", &ProblemData::Client::twLate);

    py::class_<ProblemData::Route>(m, "Route")
        .def(py::init<size_t>(), py::arg("vehicle_capacity"))
        .def_readonly("vehicle_capacity", &ProblemData::Route::vehicleCapacity);

    py::class_<ProblemData>(m, "ProblemData")
        .def(py::init<std::vector<std::pair<int, int>> const &,
                      std::vector<int> const &,
                      std::vector<size_t> const &,
                      std::vector<std::pair<int, int>> const &,
                      std::vector<int> const &,
                      std::vector<std::vector<int>> const &>(),
             py::arg("coords"),
             py::arg("demands"),
             py::arg("vehicle_capacities"),
             py::arg("time_windows"),
             py::arg("service_durations"),
             py::arg("duration_matrix"))
        .def_property_readonly("num_clients", &ProblemData::numClients)
        .def_property_readonly("max_num_routes", &ProblemData::maxNumRoutes)
        .def("client",
             &ProblemData::client,
             py::arg("client"),
             py::return_value_policy::reference)
        .def("depot", &ProblemData::depot, py::return_value_policy::reference)
        .def("route",
             &ProblemData::route,
             py::arg("route"),
             py::return_value_policy::reference)
        .def("dist", &ProblemData::dist, py::arg("first"), py::arg("second"))
        .def("distance_matrix",
             &ProblemData::distanceMatrix,
             py::return_value_policy::reference);
}
