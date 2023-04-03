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

    py::class_<ProblemData>(m, "ProblemData")
        .def(py::init<
                 std::vector<std::pair<distance_type, distance_type>> const &,
                 std::vector<int> const &,
                 size_t,
                 size_t,
                 std::vector<std::pair<duration_type, duration_type>> const &,
                 std::vector<duration_type> const &,
                 std::vector<std::vector<distance_type>> const &>(),
             py::arg("coords"),
             py::arg("demands"),
             py::arg("nb_vehicles"),
             py::arg("vehicle_cap"),
             py::arg("time_windows"),
             py::arg("service_durations"),
             py::arg("duration_matrix"))
        .def_property_readonly("num_clients", &ProblemData::numClients)
        .def_property_readonly("num_vehicles", &ProblemData::numVehicles)
        .def_property_readonly("vehicle_capacity",
                               &ProblemData::vehicleCapacity)
        .def("client",
             &ProblemData::client,
             py::arg("client"),
             py::return_value_policy::reference_internal)
        .def("depot", &ProblemData::depot, py::return_value_policy::reference)
        .def("dist", &ProblemData::dist, py::arg("first"), py::arg("second"))
        .def("duration",
             &ProblemData::duration,
             py::arg("first"),
             py::arg("second"))
        .def("distance_matrix",
             &ProblemData::distanceMatrix,
             py::return_value_policy::reference_internal)
        .def("duration_matrix",
             &ProblemData::durationMatrix,
             py::return_value_policy::reference_internal);
}
