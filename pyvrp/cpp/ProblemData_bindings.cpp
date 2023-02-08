#include "ProblemData.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(ProblemData, m)
{
    py::class_<ProblemData::Client>(m, "Client")
        .def_readonly("x", &ProblemData::Client::x)
        .def_readonly("y", &ProblemData::Client::y)
        .def_readonly("serv_dur", &ProblemData::Client::servDur)
        .def_readonly("demand", &ProblemData::Client::demand)
        .def_readonly("tw_early", &ProblemData::Client::twEarly)
        .def_readonly("tw_late", &ProblemData::Client::twLate)
        .def_readonly("release_time", &ProblemData::Client::releaseTime);

    py::class_<ProblemData>(m, "ProblemData")
        .def(py::init<std::vector<std::pair<TDist, TDist>> const &,
                      std::vector<int> const &,
                      int,
                      int,
                      std::vector<std::pair<TTime, TTime>> const &,
                      std::vector<TTime> const &,
                      std::vector<std::vector<TDist>> const &,
                      std::vector<TTime> const &>(),
             py::arg("coords"),
             py::arg("demands"),
             py::arg("nb_vehicles"),
             py::arg("vehicle_cap"),
             py::arg("time_windows"),
             py::arg("service_durations"),
             py::arg("distance_matrix"),
             py::arg("release_times"))
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
        .def("duration", &ProblemData::duration, py::arg("first"), py::arg("second"))
        .def("distance_matrix",
             &ProblemData::distanceMatrix,
             py::return_value_policy::reference)
        .def("duration_matrix",
             &ProblemData::durationMatrix,
             py::return_value_policy::reference)
        .def_static("from_file", &ProblemData::fromFile, py::arg("where"));
}
