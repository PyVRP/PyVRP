#include "ProblemData.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_ProblemData(py::module_ &m)
{
    py::class_<ProblemData>(m, "ProblemData")
        .def(py::init<std::vector<std::pair<int, int>> const &,
                      std::vector<int> const &,
                      int,
                      int,
                      std::vector<std::pair<int, int>> const &,
                      std::vector<int> const &,
                      std::vector<std::vector<int>> const &,
                      std::vector<int> const &>(),
             py::arg("coords"),
             py::arg("demands"),
             py::arg("nb_vehicles"),
             py::arg("vehicle_cap"),
             py::arg("time_windows"),
             py::arg("service_durations"),
             py::arg("duration_matrix"),
             py::arg("release_times"))
        .def("client", &ProblemData::client)
        .def("depot", &ProblemData::depot)
        .def("dist", &ProblemData::dist)
        .def("distance_matrix",
             &ProblemData::distanceMatrix,
             py::return_value_policy::reference)
        .def("num_clients", &ProblemData::numClients)
        .def("num_vehicles", &ProblemData::numVehicles)
        .def("vehicle_capacity", &ProblemData::vehicleCapacity)
        .def_static("from_file", &ProblemData::fromFile);
}
